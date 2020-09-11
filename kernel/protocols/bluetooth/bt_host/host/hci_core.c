/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#include "common/log.h"

#include "common/rpa.h"
#include "keys.h"
#include "monitor.h"
#include "hci_core.h"
#include "hci_ecc.h"
#include "ecc.h"

#include "conn_internal.h"
#include "l2cap_internal.h"
#include "gatt_internal.h"
#include "smp.h"
#include "crypto.h"
#include "settings.h"
#include "soc.h"
#include <hci_api.h>

/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT  K_SECONDS(5)
#define RPA_TIMEOUT		  K_SECONDS(CONFIG_BT_RPA_TIMEOUT)

/* Stacks for the threads */
#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
static struct k_thread rx_thread_data;
static BT_STACK_NOINIT(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);
#endif

static void init_work(struct k_work *work);


struct bt_dev bt_dev;

static bt_ready_cb_t ready_cb;

static bt_le_scan_cb_t *scan_dev_found_cb;

static u8_t pub_key[64];
static struct bt_pub_key_cb *pub_key_cb;
static bt_dh_key_cb_t dh_key_cb;
extern struct k_sem g_poll_sem;

#if defined(CONFIG_BT_BREDR)
static bt_br_discovery_cb_t *discovery_cb;
struct bt_br_discovery_result *discovery_results;
static size_t discovery_results_size;
static size_t discovery_results_count;
#endif /* CONFIG_BT_BREDR */

struct acl_data {
	/** BT_BUF_ACL_IN */
	u8_t  type;

	/* Index into the bt_conn storage array */
	u8_t  id;

	/** ACL connection handle */
	u16_t handle;
};

#define acl(buf) ((struct acl_data *)net_buf_user_data(buf))

NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
			BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

int hci_vs_set_bd_add(const bt_addr_t *addr);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static void report_completed_packet(struct net_buf *buf)
{
	u16_t handle = acl(buf)->handle;
	struct bt_conn *conn;

	net_buf_destroy(buf);

	/* Do nothing if controller to host flow control is not supported */
	if (!(bt_dev.supported_commands[10] & 0x20)) {
		return;
	}

	conn = bt_conn_lookup_id(acl(buf)->id);
	if (!conn) {
		BT_WARN("Unable to look up conn with id 0x%02x", acl(buf)->id);
		return;
	}

	if (conn->state != BT_CONN_CONNECTED &&
		conn->state != BT_CONN_DISCONNECT) {
		BT_WARN("Not reporting packet for non-connected conn");
		bt_conn_unref(conn);
		return;
	}

	bt_conn_unref(conn);

	BT_DBG("Reporting completed packet for handle %u", handle);

	struct handle_count hc;
	hc.handle = handle;
	hc.count = 1;

	hci_api_host_num_complete_packets(1, &hc);
}

#define ACL_IN_SIZE BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_RX_MTU)
NET_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE,
			BT_BUF_USER_DATA_MIN, report_completed_packet);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

#if defined(CONFIG_BT_OBSERVER) || defined(CONFIG_BT_CONN)
static const bt_addr_le_t *find_id_addr(u8_t id, const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys;

		keys = bt_keys_find_irk(id, addr);
		if (keys) {
			BT_INFO("Identity %s matched RPA %s",
				   bt_addr_le_str(&keys->addr),
				   bt_addr_le_str(addr));
			return &keys->addr;
		}
	}

	return addr;
}
#endif /* CONFIG_BT_OBSERVER || CONFIG_BT_CONN */

static int set_advertise_enable(bool enable)
{
	int err;
	u8_t adv_enable;

	if (enable) {
		adv_enable = BT_HCI_LE_ADV_ENABLE;
	} else {
		adv_enable = BT_HCI_LE_ADV_DISABLE;
	}

	err = hci_api_le_adv_enable(adv_enable);
	if (err) {
		return err;
	}

	if (enable) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	}

	return 0;
}

static int set_random_address( bt_addr_t *addr)
{
	int err;

	BT_DBG("%s", bt_addr_str(addr));

	/* Do nothing if we already have the right address */
	if (!bt_addr_cmp(addr, &bt_dev.random_addr.a)) {
		return 0;
	}

	err = hci_api_le_set_random_addr(addr->val);
	if (err) {
		return err;
	}

	bt_addr_copy(&bt_dev.random_addr.a, addr);
	bt_dev.random_addr.type = BT_ADDR_LE_RANDOM;
	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
/* this function sets new RPA only if current one is no longer valid */
int le_set_private_addr(u8_t id)
{
	bt_addr_t rpa;
	int err;

	/* check if RPA is valid */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID)) {
		return 0;
	}

	err = bt_rpa_create(bt_dev.irk[id], &rpa);
	if (!err) {
		err = set_random_address(&rpa);
		if (!err) {
			atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);
		}
	}

	/* restart timer even if failed to set new RPA */
	k_delayed_work_submit(&bt_dev.rpa_update, RPA_TIMEOUT);

	return err;
}

static void rpa_timeout(struct k_work *work)
{
	BT_DBG("");

	/* Invalidate RPA */
	atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	/*
	 * we need to update rpa only if advertising is ongoing, with
	 * BT_DEV_KEEP_ADVERTISING flag is handled in disconnected event
	 */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		/* make sure new address is used */
		set_advertise_enable(false);
		le_set_private_addr(bt_dev.adv_id);
		set_advertise_enable(true);
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
		/* TODO do we need to toggle scan? */
		le_set_private_addr(BT_ID_DEFAULT);
	}
}
#else
int le_set_private_addr(u8_t id)
{
	bt_addr_t nrpa;
	int err;

	err = bt_rand(nrpa.val, sizeof(nrpa.val));
	if (err) {
		return err;
	}

	nrpa.val[5] &= 0x3f;

	return set_random_address(&nrpa);
}
#endif

#if defined(CONFIG_BT_OBSERVER)
static int set_le_scan_enable(u8_t enable)
{
	int err;

	err = hci_api_le_scan_enable(enable, (enable == BT_HCI_LE_SCAN_ENABLE)?atomic_test_bit(bt_dev.flags,
						 BT_DEV_SCAN_FILTER_DUP): BT_HCI_LE_SCAN_FILTER_DUP_DISABLE);
	if (err)
	{
		return err;
	}

	if (enable == BT_HCI_LE_SCAN_ENABLE) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);
	}

	return 0;
}
#endif /* CONFIG_BT_OBSERVER */

void bt_hci_conn_notify_tx(u16_t handle)
{
	struct bt_conn *conn;
	conn = bt_conn_lookup_handle(handle);
	if(!conn){
		return;
	}
	bt_conn_notify_tx(conn);
	bt_conn_unref(conn);
	return;
}

void bt_hci_num_complete_packets(u16_t handle, uint16 count)
{
	struct bt_conn *conn;
	unsigned int key;
	key = irq_lock();

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("No connection for handle %u", handle);
		irq_unlock(key);
		return;
	}

	irq_unlock(key);

	while (count--) {
		sys_snode_t *node;

		key = irq_lock();
		node = sys_slist_get(&conn->tx_pending);
		irq_unlock(key);

		if (!node) {
			BT_ERR("packets count mismatch");
			break;
		}

		k_fifo_put(&conn->tx_notify, node);
		k_sem_give(bt_conn_get_pkts(conn));
	}

	bt_conn_unref(conn);

	return;
}

#if defined(CONFIG_BT_CONN)
void hci_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr = (void *)buf->data;
	u16_t handle, len = sys_le16_to_cpu(hdr->len);
	struct bt_conn *conn;
	u8_t flags;

	BT_DBG("buf %p", buf);

	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_acl_flags(handle);

	acl(buf)->handle = bt_acl_handle(handle);
	acl(buf)->id = BT_CONN_ID_INVALID;

	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("handle %u len %u flags %u", acl(buf)->handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ACL data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	conn = bt_conn_lookup_handle(acl(buf)->handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", acl(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	acl(buf)->id = bt_conn_get_id(conn);

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_CENTRAL)
static int hci_le_create_conn( struct bt_conn *conn)
{
	return hci_api_le_create_conn(BT_GAP_SCAN_FAST_INTERVAL,
								BT_GAP_SCAN_FAST_INTERVAL,
								0,
								conn->le.resp_addr.type,
								conn->le.resp_addr.a.val,
								conn->le.init_addr.type,
								conn->le.interval_min,
								conn->le.interval_max,
								conn->le.latency,
								conn->le.timeout,
								0,
								0);
}
#endif /* CONFIG_BT_CENTRAL */

void hci_stack_dump(const struct k_thread *thread, void *user_data)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	stack_analyze((char *)user_data, (char *)thread->stack_info.start,
						thread->stack_info.size);
#endif
}

static void hci_disconn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u reason %u", evt->status, handle,
		   evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		goto advertise;
	}

	conn->err = evt->reason;

	/* Check stacks usage (no-ops if not enabled) */
	k_thread_foreach(hci_stack_dump, "HCI");
#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	STACK_ANALYZE("rx stack", rx_thread_stack);
#endif
	//STACK_ANALYZE("tx stack", tx_thread_stack);

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	conn->handle = 0;

	if (conn->type != BT_CONN_TYPE_LE) {
#if defined(CONFIG_BT_BREDR)
		if (conn->type == BT_CONN_TYPE_SCO) {
			bt_sco_cleanup(conn);
			return;
		}
		/*
		 * If only for one connection session bond was set, clear keys
		 * database row for this connection.
		 */
		if (conn->type == BT_CONN_TYPE_BR &&
			atomic_test_and_clear_bit(conn->flags, BT_CONN_BR_NOBOND)) {
			bt_keys_link_key_clear(conn->br.link_key);
		}
#endif
		bt_conn_unref(conn);
		return;
	}

#if defined(CONFIG_BT_CENTRAL)
	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		bt_le_scan_update(false);
	}
#endif /* CONFIG_BT_CENTRAL */

	bt_conn_unref(conn);

advertise:
	if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
		!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			!BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			le_set_private_addr(bt_dev.adv_id);
		}

		set_advertise_enable(true);
	}
}

static int hci_le_set_data_len(struct bt_conn *conn)
{
	u16_t tx_octets, tx_time;
	int err;

	err = hci_api_le_get_max_data_len(&tx_octets, &tx_time);
	if (err) {
		return err;
	}

	err = hci_api_le_set_data_len(conn->handle, tx_octets, tx_time);
	if (err) {
		return err;
	}

	return 0;
}

static int hci_le_set_phy(struct bt_conn *conn)
{
	return hci_api_le_set_phy(conn->handle, 0, BT_HCI_LE_PHY_PREFER_2M,
								BT_HCI_LE_PHY_PREFER_2M,BT_HCI_LE_PHY_CODED_ANY);
}

static void update_conn_param(struct bt_conn *conn)
{
	/*
	 * Core 4.2 Vol 3, Part C, 9.3.12.2
	 * The Peripheral device should not perform a Connection Parameter
	 * Update procedure within 5 s after establishing a connection.
	 */
	k_delayed_work_submit(&conn->le.update_work,
				 conn->role == BT_HCI_ROLE_MASTER ? K_NO_WAIT :
				 CONN_UPDATE_TIMEOUT);
}

#if defined(CONFIG_BT_SMP)
static void update_pending_id(struct bt_keys *keys, void *data)
{
	if (keys->flags & BT_KEYS_ID_PENDING_ADD) {
		keys->flags &= ~BT_KEYS_ID_PENDING_ADD;
		bt_id_add(keys);
		return;
	}

	if (keys->flags & BT_KEYS_ID_PENDING_DEL) {
		keys->flags &= ~BT_KEYS_ID_PENDING_DEL;
		bt_id_del(keys);
		return;
	}
}
#endif

static void le_enh_conn_complete(struct bt_hci_evt_le_enh_conn_complete *evt)
{
	u16_t handle = sys_le16_to_cpu(evt->handle);
	bt_addr_le_t peer_addr, id_addr;
	struct bt_conn *conn;
	int err;

	BT_DBG("status %u handle %u role %u %s", evt->status, handle,
		   evt->role, bt_addr_le_str(&evt->peer_addr));

#if defined(CONFIG_BT_SMP)
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_ID_PENDING)) {
		bt_keys_foreach(BT_KEYS_IRK, update_pending_id, NULL);
	}
#endif

	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_DIRECT_ADVERTISING))
	{
		if (evt->status == BT_HCI_ERR_ADV_TIMEOUT)
		{
			atomic_clear_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);
			atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);
            extern void ble_adv_timeout_cb();
			ble_adv_timeout_cb();
		}
	}
	if (evt->status) {
		/*
		 * if there was an error we are only interested in pending
		 * connection so there is no need to check ID address as
		 * only one connection can be in that state
		 *
		 * Depending on error code address might not be valid anyway.
		 */
		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
		if (!conn) {
			return;
		}

		conn->err = evt->status;

		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		/* Drop the reference got by lookup call in CONNECT state.
		 * We are now in DISCONNECTED state since no successful LE
		 * link been made.
		 */
		bt_conn_unref(conn);

		return;
	}

	bt_addr_le_copy(&id_addr, &evt->peer_addr);

	/* Translate "enhanced" identity address type to normal one */
	if (id_addr.type == BT_ADDR_LE_PUBLIC_ID ||
		id_addr.type == BT_ADDR_LE_RANDOM_ID) {
		id_addr.type -= BT_ADDR_LE_PUBLIC_ID;
		bt_addr_copy(&peer_addr.a, &evt->peer_rpa);
		peer_addr.type = BT_ADDR_LE_RANDOM;
	} else {
		bt_addr_le_copy(&peer_addr, &evt->peer_addr);
	}

	/*
	 * Make lookup to check if there's a connection object in
	 * CONNECT state associated with passed peer LE address.
	 */
	conn = bt_conn_lookup_state_le(&id_addr, BT_CONN_CONNECT);

	if (evt->role == BT_CONN_ROLE_SLAVE) {
		/*
		 * clear advertising even if we are not able to add connection
		 * object to keep host in sync with controller state
		 */
		atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);

		/* only for slave we may need to add new connection */
		if (!conn) {
			conn = bt_conn_add_le(&id_addr);
		}
	}

	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u", handle);
		return;
	}

	conn->handle = handle;
	bt_addr_le_copy(&conn->le.dst, &id_addr);
	conn->le.interval = sys_le16_to_cpu(evt->interval);
	conn->le.latency = sys_le16_to_cpu(evt->latency);
	conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
	conn->role = evt->role;

	/*
	 * Use connection address (instead of identity address) as initiator
	 * or responder address. Only slave needs to be updated. For master all
	 * was set during outgoing connection creation.
	 */
	if (conn->role == BT_HCI_ROLE_SLAVE) {
		conn->id = bt_dev.adv_id;
		bt_addr_le_copy(&conn->le.init_addr, &peer_addr);

		if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
			bt_addr_copy(&conn->le.resp_addr.a, &evt->local_rpa);
			conn->le.resp_addr.type = BT_ADDR_LE_RANDOM;
		} else {
			bt_addr_le_copy(&conn->le.resp_addr,
					&bt_dev.id_addr[conn->id]);
		}

		/* if the controller supports, lets advertise for another
		 * slave connection.
		 * check for connectable advertising state is sufficient as
		 * this is how this le connection complete for slave occurred.
		 */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
			BT_LE_STATES_SLAVE_CONN_ADV(bt_dev.le.states)) {
			if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
				le_set_private_addr(bt_dev.adv_id);
			}

			set_advertise_enable(true);
		}
	}

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	/*
	 * it is possible that connection was disconnected directly from
	 * connected callback so we must check state before doing connection
	 * parameters update
	 */
	if (conn->state != BT_CONN_CONNECTED) {
		goto done;
	}

	if ((evt->role == BT_HCI_ROLE_MASTER) ||
		BT_FEAT_LE_SLAVE_FEATURE_XCHG(bt_dev.le.features)) {
		err = hci_api_le_read_remote_features(conn->handle);
		if (!err) {
			goto done;
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
		BT_FEAT_LE_PHY_2M(bt_dev.le.features)) {
		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	/* only set when local DLE feature and remote DLE feature is enabled */
#if 0
	if (BT_FEAT_LE_DLE(bt_dev.le.features)) {
		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}
#endif

	//update_conn_param(conn);

done:
	bt_conn_unref(conn);
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		bt_le_scan_update(false);
	}
}

static void le_legacy_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	struct bt_hci_evt_le_enh_conn_complete enh;
	const bt_addr_le_t *id_addr;

	BT_DBG("status %u role %u %s", evt->status, evt->role,
		   bt_addr_le_str(&evt->peer_addr));

	enh.status		 = evt->status;
	enh.handle		 = evt->handle;
	enh.role		   = evt->role;
	enh.interval	   = evt->interval;
	enh.latency		= evt->latency;
	enh.supv_timeout   = evt->supv_timeout;
	enh.clock_accuracy = evt->clock_accuracy;

	bt_addr_le_copy(&enh.peer_addr, &evt->peer_addr);

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		bt_addr_copy(&enh.local_rpa, &bt_dev.random_addr.a);
	} else {
		bt_addr_copy(&enh.local_rpa, BT_ADDR_ANY);
	}

	if (evt->role == BT_HCI_ROLE_SLAVE) {
		id_addr = find_id_addr(bt_dev.adv_id, &enh.peer_addr);
	} else {
		id_addr = find_id_addr(BT_ID_DEFAULT, &enh.peer_addr);
	}

	if (id_addr != &enh.peer_addr) {
		bt_addr_copy(&enh.peer_rpa, &enh.peer_addr.a);
		bt_addr_le_copy(&enh.peer_addr, id_addr);
		enh.peer_addr.type += BT_ADDR_LE_PUBLIC_ID;
	} else {
		bt_addr_copy(&enh.peer_rpa, BT_ADDR_ANY);
	}

	le_enh_conn_complete(&enh);
}

static void le_remote_feat_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_remote_feat_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		memcpy(conn->le.features, evt->features,
			   sizeof(conn->le.features));
	}

	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
		BT_FEAT_LE_PHY_2M(bt_dev.le.features) &&
		BT_FEAT_LE_PHY_2M(conn->le.features)) {
		int err;

		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	if (BT_FEAT_LE_DLE(bt_dev.le.features) &&
		BT_FEAT_LE_DLE(conn->le.features)) {
		int err;

		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
}

static void le_data_len_change(struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *evt = (void *)buf->data;
	u16_t max_tx_octets = sys_le16_to_cpu(evt->max_tx_octets);
	u16_t max_rx_octets = sys_le16_to_cpu(evt->max_rx_octets);
	u16_t max_tx_time = sys_le16_to_cpu(evt->max_tx_time);
	u16_t max_rx_time = sys_le16_to_cpu(evt->max_rx_time);
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	BT_DBG("max. tx: %u (%uus), max. rx: %u (%uus)", max_tx_octets,
		   max_tx_time, max_rx_octets, max_rx_time);

	bt_dev.le.mtu = max_tx_octets;

	if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_AUTO_DATA_LEN)) {
		goto done;
	}

	update_conn_param(conn);

	(void)max_tx_octets;
	(void)max_rx_octets;
	(void)max_tx_time;
	(void)max_rx_time;
done:
	bt_conn_unref(conn);
}

static void le_phy_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_phy_update_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	BT_DBG("PHY updated: status: 0x%x, tx: %u, rx: %u",
		   evt->status, evt->tx_phy, evt->rx_phy);

	if (!IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) ||
		!atomic_test_and_clear_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE)) {
		goto done;
	}

	if (BT_FEAT_LE_DLE(bt_dev.le.features) &&
		BT_FEAT_LE_DLE(conn->le.features)) {
		int err;

		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
}

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param)
{
	/* All limits according to BT Core spec 5.0 [Vol 2, Part E, 7.8.12] */

	if (param->interval_min > param->interval_max ||
		param->interval_min < 6 || param->interval_max > 3200) {
		return false;
	}

	if (param->latency > 499) {
		return false;
	}

	if (param->timeout < 10 || param->timeout > 3200 ||
		((4 * param->timeout) <=
		 ((1 + param->latency) * param->interval_max))) {
		return false;
	}

	return true;
}

static int le_conn_param_neg_reply(u16_t handle, u8_t reason)
{
	return hci_api_le_conn_param_neg_reply(handle, reason);
}

static int le_conn_param_req_reply(u16_t handle,
				   const struct bt_le_conn_param *param)
{
	return hci_api_le_conn_param_req_reply(handle, param->interval_min,param->interval_max, 
		param->latency,param->timeout, 0, 0 );
}

static int le_conn_param_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *evt = (void *)buf->data;
	struct bt_le_conn_param param;
	struct bt_conn *conn;
	u16_t handle;
	int err;

	handle = sys_le16_to_cpu(evt->handle);
	param.interval_min = sys_le16_to_cpu(evt->interval_min);
	param.interval_max = sys_le16_to_cpu(evt->interval_max);
	param.latency = sys_le16_to_cpu(evt->latency);
	param.timeout = sys_le16_to_cpu(evt->timeout);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return le_conn_param_neg_reply(handle,
						   BT_HCI_ERR_UNKNOWN_CONN_ID);
	}

	if (!le_param_req(conn, &param)) {
		err = le_conn_param_neg_reply(handle,
						  BT_HCI_ERR_INVALID_LL_PARAM);
	} else {
		err = le_conn_param_req_reply(handle, &param);
	}

	bt_conn_unref(conn);
	return err;
}

static void le_conn_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		conn->le.interval = sys_le16_to_cpu(evt->interval);
		conn->le.latency = sys_le16_to_cpu(evt->latency);
		conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
		notify_le_param_updated(conn);
	}

	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_CENTRAL)
static void check_pending_conn( bt_addr_le_t *id_addr,
					bt_addr_le_t *addr, u8_t evtype)
{
	struct bt_conn *conn;

	/* No connections are allowed during explicit scanning */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return;
	}

	/* Return if event is not connectable */
	if (evtype != BT_LE_ADV_IND && evtype != BT_LE_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_state_le(id_addr, BT_CONN_CONNECT_SCAN);
	if (!conn) {
		return;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
		set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE)) {
		goto failed;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		if (le_set_private_addr(BT_ID_DEFAULT)) {
			goto failed;
		}

		bt_addr_le_copy(&conn->le.init_addr, &bt_dev.random_addr);
	} else {
		bt_addr_le_t *addr = &bt_dev.id_addr[conn->id];

		/* If Static Random address is used as Identity address we
		 * need to restore it before creating connection. Otherwise
		 * NRPA used for active scan could be used for connection.
		 */
		if (addr->type == BT_ADDR_LE_RANDOM) {
			set_random_address(&addr->a);
		}

		bt_addr_le_copy(&conn->le.init_addr, addr);
	}

	bt_addr_le_copy(&conn->le.resp_addr, addr);

	if (hci_le_create_conn(conn)) {
		goto failed;
	}

	bt_conn_set_state(conn, BT_CONN_CONNECT);
	bt_conn_unref(conn);
	return;

failed:
	conn->err = BT_HCI_ERR_UNSPECIFIED;
	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	bt_conn_unref(conn);
	bt_le_scan_update(false);
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static int set_flow_control(void)
{
	int err;

	/* Check if host flow control is actually supported */
	if (!(bt_dev.supported_commands[10] & 0x20)) {
		BT_WARN("Controller to host flow control not supported");
		return 0;
	}

	err = hci_api_set_host_buffer_size(CONFIG_BT_L2CAP_RX_MTU+sizeof(struct bt_l2cap_hdr), 0, CONFIG_BT_ACL_RX_COUNT, 0);
	if (err) {
		return err;
	}

	return hci_api_set_host_flow_enable(BT_HCI_CTL_TO_HOST_FLOW_ENABLE);
}
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

static int bt_clear_all_pairings(u8_t id)
{
	bt_conn_disconnect_all(id);

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		bt_keys_clear_all(id);
	}

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		bt_keys_link_key_clear_addr(NULL);
	}

	return 0;
}

int bt_unpair(u8_t id, const bt_addr_le_t *addr)
{
	struct bt_conn *conn;

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (!addr || !bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		return bt_clear_all_pairings(id);
	}

	conn = bt_conn_lookup_addr_le(id, addr);
	if (conn) {
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(conn);
	}

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		/* LE Public may indicate BR/EDR as well */
		if (addr->type == BT_ADDR_LE_PUBLIC) {
			bt_keys_link_key_clear_addr(&addr->a);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(id, addr);
		if (keys) {
			bt_keys_clear(keys);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_gatt_clear_ccc(id, addr);
	}

	return 0;
}

#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_BREDR)
static void reset_pairing(struct bt_conn *conn)
{
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR);
	atomic_clear_bit(conn->flags, BT_CONN_BR_LEGACY_SECURE);

	/* Reset required security level to current operational */
	conn->required_sec_level = conn->sec_level;
}

static int reject_conn(const bt_addr_t *bdaddr, u8_t reason)
{
	struct bt_hci_cp_reject_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_REJECT_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->reason = reason;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_REJECT_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int accept_sco_conn(const bt_addr_t *bdaddr, struct bt_conn *sco_conn)
{
	struct bt_hci_cp_accept_sync_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->pkt_type = sco_conn->sco.pkt_type;
	cp->tx_bandwidth = 0x00001f40;
	cp->rx_bandwidth = 0x00001f40;
	cp->max_latency = 0x0007;
	cp->retrans_effort = 0x01;
	cp->content_format = BT_VOICE_CVSD_16BIT;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int accept_conn(const bt_addr_t *bdaddr)
{
	struct bt_hci_cp_accept_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_ACCEPT_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->role = BT_HCI_ROLE_SLAVE;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_ACCEPT_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static void bt_esco_conn_req(struct bt_hci_evt_conn_request *evt)
{
	struct bt_conn *sco_conn;

	sco_conn = bt_conn_add_sco(&evt->bdaddr, evt->link_type);
	if (!sco_conn) {
		reject_conn(&evt->bdaddr, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	if (accept_sco_conn(&evt->bdaddr, sco_conn)) {
		BT_ERR("Error accepting connection from %s",
			   bt_addr_str(&evt->bdaddr));
		reject_conn(&evt->bdaddr, BT_HCI_ERR_UNSPECIFIED);
		bt_sco_cleanup(sco_conn);
		return;
	}

	sco_conn->role = BT_HCI_ROLE_SLAVE;
	bt_conn_set_state(sco_conn, BT_CONN_CONNECT);
	bt_conn_unref(sco_conn);
}

static void conn_req(struct net_buf *buf)
{
	struct bt_hci_evt_conn_request *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("conn req from %s, type 0x%02x", bt_addr_str(&evt->bdaddr),
		   evt->link_type);

	if (evt->link_type != BT_HCI_ACL) {
		bt_esco_conn_req(evt);
		return;
	}

	conn = bt_conn_add_br(&evt->bdaddr);
	if (!conn) {
		reject_conn(&evt->bdaddr, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	accept_conn(&evt->bdaddr);
	conn->role = BT_HCI_ROLE_SLAVE;
	bt_conn_set_state(conn, BT_CONN_CONNECT);
	bt_conn_unref(conn);
}

static void update_sec_level_br(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	if (conn->br.link_key) {
		if (conn->br.link_key->flags & BT_LINK_KEY_AUTHENTICATED) {
			if (conn->encrypt == 0x02) {
				conn->sec_level = BT_SECURITY_FIPS;
			} else {
				conn->sec_level = BT_SECURITY_HIGH;
			}
		} else {
			conn->sec_level = BT_SECURITY_MEDIUM;
		}
	} else {
		BT_WARN("No BR/EDR link key found");
		conn->sec_level = BT_SECURITY_MEDIUM;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}

static void synchronous_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_sync_conn_complete *evt = (void *)buf->data;
	struct bt_conn *sco_conn;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x, handle %u, type 0x%02x", evt->status, handle,
		   evt->link_type);

	sco_conn = bt_conn_lookup_addr_sco(&evt->bdaddr);
	if (!sco_conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		sco_conn->err = evt->status;
		bt_conn_set_state(sco_conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(sco_conn);
		return;
	}

	sco_conn->handle = handle;
	bt_conn_set_state(sco_conn, BT_CONN_CONNECTED);
	bt_conn_unref(sco_conn);
}

static void conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_conn_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	struct bt_hci_cp_read_remote_features *cp;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x, handle %u, type 0x%02x", evt->status, handle,
		   evt->link_type);

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		conn->err = evt->status;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);
		return;
	}

	conn->handle = handle;
	conn->encrypt = evt->encr_enabled;
	update_sec_level_br(conn);
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_conn_unref(conn);

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_REMOTE_FEATURES, sizeof(*cp));
	if (!buf) {
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = evt->handle;

	bt_hci_cmd_send_sync(BT_HCI_OP_READ_REMOTE_FEATURES, buf, NULL);
}

static void pin_code_req(struct net_buf *buf)
{
	struct bt_hci_evt_pin_code_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_pin_code_req(conn);
	bt_conn_unref(conn);
}

static void link_key_notify(struct net_buf *buf)
{
	struct bt_hci_evt_link_key_notify *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	BT_DBG("%s, link type 0x%02x", bt_addr_str(&evt->bdaddr), evt->key_type);

	if (!conn->br.link_key) {
		conn->br.link_key = bt_keys_get_link_key(&evt->bdaddr);
	}
	if (!conn->br.link_key) {
		BT_ERR("Can't update keys for %s", bt_addr_str(&evt->bdaddr));
		bt_conn_unref(conn);
		return;
	}

	/* clear any old Link Key flags */
	conn->br.link_key->flags = 0;

	switch (evt->key_type) {
	case BT_LK_COMBINATION:
		/*
		 * Setting Combination Link Key as AUTHENTICATED means it was
		 * successfully generated by 16 digits wide PIN code.
		 */
		if (atomic_test_and_clear_bit(conn->flags,
						  BT_CONN_BR_LEGACY_SECURE)) {
			conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		}
		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P192:
		conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P192:
		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P256:
		conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P256:
		conn->br.link_key->flags |= BT_LINK_KEY_SC;

		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	default:
		BT_WARN("Unsupported Link Key type %u", evt->key_type);
		memset(conn->br.link_key->val, 0,
			   sizeof(conn->br.link_key->val));
		break;
	}

	bt_conn_unref(conn);
}

static void link_key_neg_reply(const bt_addr_t *bdaddr)
{
	struct bt_hci_cp_link_key_neg_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_LINK_KEY_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	bt_hci_cmd_send_sync(BT_HCI_OP_LINK_KEY_NEG_REPLY, buf, NULL);
}

static void link_key_reply(const bt_addr_t *bdaddr, const u8_t *lk)
{
	struct bt_hci_cp_link_key_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_LINK_KEY_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	memcpy(cp->link_key, lk, 16);
	bt_hci_cmd_send_sync(BT_HCI_OP_LINK_KEY_REPLY, buf, NULL);
}

static void link_key_req(struct net_buf *buf)
{
	struct bt_hci_evt_link_key_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("%s", bt_addr_str(&evt->bdaddr));

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		link_key_neg_reply(&evt->bdaddr);
		return;
	}

	if (!conn->br.link_key) {
		conn->br.link_key = bt_keys_find_link_key(&evt->bdaddr);
	}

	if (!conn->br.link_key) {
		link_key_neg_reply(&evt->bdaddr);
		bt_conn_unref(conn);
		return;
	}

	/*
	 * Enforce regenerate by controller stronger link key since found one
	 * in database not covers requested security level.
	 */
	if (!(conn->br.link_key->flags & BT_LINK_KEY_AUTHENTICATED) &&
		conn->required_sec_level > BT_SECURITY_MEDIUM) {
		link_key_neg_reply(&evt->bdaddr);
		bt_conn_unref(conn);
		return;
	}

	link_key_reply(&evt->bdaddr, conn->br.link_key->val);
	bt_conn_unref(conn);
}

static void io_capa_neg_reply(const bt_addr_t *bdaddr, const u8_t reason)
{
	struct bt_hci_cp_io_capability_neg_reply *cp;
	struct net_buf *resp_buf;

	resp_buf = bt_hci_cmd_create(BT_HCI_OP_IO_CAPABILITY_NEG_REPLY,
					 sizeof(*cp));
	if (!resp_buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(resp_buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->reason = reason;
	bt_hci_cmd_send_sync(BT_HCI_OP_IO_CAPABILITY_NEG_REPLY, resp_buf, NULL);
}

static void io_capa_resp(struct net_buf *buf)
{
	struct bt_hci_evt_io_capa_resp *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("remote %s, IOcapa 0x%02x, auth 0x%02x",
		   bt_addr_str(&evt->bdaddr), evt->capability, evt->authentication);

	if (evt->authentication > BT_HCI_GENERAL_BONDING_MITM) {
		BT_ERR("Invalid remote authentication requirements");
		io_capa_neg_reply(&evt->bdaddr,
				  BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	if (evt->capability > BT_IO_NO_INPUT_OUTPUT) {
		BT_ERR("Invalid remote io capability requirements");
		io_capa_neg_reply(&evt->bdaddr,
				  BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	conn->br.remote_io_capa = evt->capability;
	conn->br.remote_auth = evt->authentication;
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	bt_conn_unref(conn);
}

static void io_capa_req(struct net_buf *buf)
{
	struct bt_hci_evt_io_capa_req *evt = (void *)buf->data;
	struct net_buf *resp_buf;
	struct bt_conn *conn;
	struct bt_hci_cp_io_capability_reply *cp;
	u8_t auth;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	resp_buf = bt_hci_cmd_create(BT_HCI_OP_IO_CAPABILITY_REPLY,
					 sizeof(*cp));
	if (!resp_buf) {
		BT_ERR("Out of command buffers");
		bt_conn_unref(conn);
		return;
	}

	/*
	 * Set authentication requirements when acting as pairing initiator to
	 * 'dedicated bond' with MITM protection set if local IO capa
	 * potentially allows it, and for acceptor, based on local IO capa and
	 * remote's authentication set.
	 */
	if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR)) {
		if (bt_conn_get_io_capa() != BT_IO_NO_INPUT_OUTPUT) {
			auth = BT_HCI_DEDICATED_BONDING_MITM;
		} else {
			auth = BT_HCI_DEDICATED_BONDING;
		}
	} else {
		auth = bt_conn_ssp_get_auth(conn);
	}

	cp = net_buf_add(resp_buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &evt->bdaddr);
	cp->capability = bt_conn_get_io_capa();
	cp->authentication = auth;
	cp->oob_data = 0;
	bt_hci_cmd_send_sync(BT_HCI_OP_IO_CAPABILITY_REPLY, resp_buf, NULL);
	bt_conn_unref(conn);
}

static void ssp_complete(struct net_buf *buf)
{
	struct bt_hci_evt_ssp_complete *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("status %u", evt->status);

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}

	bt_conn_unref(conn);
}

static void user_confirm_req(struct net_buf *buf)
{
	struct bt_hci_evt_user_confirm_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, sys_le32_to_cpu(evt->passkey));
	bt_conn_unref(conn);
}

static void user_passkey_notify(struct net_buf *buf)
{
	struct bt_hci_evt_user_passkey_notify *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, sys_le32_to_cpu(evt->passkey));
	bt_conn_unref(conn);
}

static void user_passkey_req(struct net_buf *buf)
{
	struct bt_hci_evt_user_passkey_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, 0);
	bt_conn_unref(conn);
}

struct discovery_priv {
	u16_t clock_offset;
	u8_t pscan_rep_mode;
	u8_t resolving;
} __packed;

static int request_name(const bt_addr_t *addr, u8_t pscan, u16_t offset)
{
	struct bt_hci_cp_remote_name_request *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_REMOTE_NAME_REQUEST, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	bt_addr_copy(&cp->bdaddr, addr);
	cp->pscan_rep_mode = pscan;
	cp->reserved = 0x00; /* reserver, should be set to 0x00 */
	cp->clock_offset = offset;

	return bt_hci_cmd_send_sync(BT_HCI_OP_REMOTE_NAME_REQUEST, buf, NULL);
}

#define EIR_SHORT_NAME		0x08
#define EIR_COMPLETE_NAME	0x09

static bool eir_has_name(const u8_t *eir)
{
	int len = 240;

	while (len) {
		if (len < 2) {
			break;
		};

		/* Look for early termination */
		if (!eir[0]) {
			break;
		}

		/* Check if field length is correct */
		if (eir[0] > len - 1) {
			break;
		}

		switch (eir[1]) {
		case EIR_SHORT_NAME:
		case EIR_COMPLETE_NAME:
			if (eir[0] > 1) {
				return true;
			}
			break;
		default:
			break;
		}

		/* Parse next AD Structure */
		len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

	return false;
}

static void report_discovery_results(void)
{
	bool resolving_names = false;
	int i;

	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (eir_has_name(discovery_results[i].eir)) {
			continue;
		}

		if (request_name(&discovery_results[i].addr,
				 priv->pscan_rep_mode, priv->clock_offset)) {
			continue;
		}

		priv->resolving = 1;
		resolving_names = true;
	}

	if (resolving_names) {
		return;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb(discovery_results, discovery_results_count);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
}

static void inquiry_complete(struct net_buf *buf)
{
	struct bt_hci_evt_inquiry_complete *evt = (void *)buf->data;

	if (evt->status) {
		BT_ERR("Failed to complete inquiry");
	}

	report_discovery_results();
}

static struct bt_br_discovery_result *get_result_slot(const bt_addr_t *addr,
							  s8_t rssi)
{
	struct bt_br_discovery_result *result = NULL;
	size_t i;

	/* check if already present in results */
	for (i = 0; i < discovery_results_count; i++) {
		if (!bt_addr_cmp(addr, &discovery_results[i].addr)) {
			return &discovery_results[i];
		}
	}

	/* Pick a new slot (if available) */
	if (discovery_results_count < discovery_results_size) {
		bt_addr_copy(&discovery_results[discovery_results_count].addr,
				 addr);
		return &discovery_results[discovery_results_count++];
	}

	/* ignore if invalid RSSI */
	if (rssi == 0xff) {
		return NULL;
	}

	/*
	 * Pick slot with smallest RSSI that is smaller then passed RSSI
	 * TODO handle TX if present
	 */
	for (i = 0; i < discovery_results_size; i++) {
		if (discovery_results[i].rssi > rssi) {
			continue;
		}

		if (!result || result->rssi > discovery_results[i].rssi) {
			result = &discovery_results[i];
		}
	}

	if (result) {
		BT_DBG("Reusing slot (old %s rssi %d dBm)",
			   bt_addr_str(&result->addr), result->rssi);

		bt_addr_copy(&result->addr, addr);
	}

	return result;
}

static void inquiry_result_with_rssi(struct net_buf *buf)
{
	struct bt_hci_evt_inquiry_result_with_rssi *evt;
	u8_t num_reports = net_buf_pull_u8(buf);

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return;
	}

	BT_DBG("number of results: %u", num_reports);

	evt = (void *)buf->data;
	while (num_reports--) {
		struct bt_br_discovery_result *result;
		struct discovery_priv *priv;

		BT_DBG("%s rssi %d dBm", bt_addr_str(&evt->addr), evt->rssi);

		result = get_result_slot(&evt->addr, evt->rssi);
		if (!result) {
			return;
		}

		priv = (struct discovery_priv *)&result->_priv;
		priv->pscan_rep_mode = evt->pscan_rep_mode;
		priv->clock_offset = evt->clock_offset;

		memcpy(result->cod, evt->cod, 3);
		result->rssi = evt->rssi;

		/* we could reuse slot so make sure EIR is cleared */
		memset(result->eir, 0, sizeof(result->eir));

		/*
		 * Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.33.
		 */
		evt = net_buf_pull(buf, sizeof(*evt));
	}
}

static void extended_inquiry_result(struct net_buf *buf)
{
	struct bt_hci_evt_extended_inquiry_result *evt = (void *)buf->data;
	struct bt_br_discovery_result *result;
	struct discovery_priv *priv;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return;
	}

	BT_DBG("%s rssi %d dBm", bt_addr_str(&evt->addr), evt->rssi);

	result = get_result_slot(&evt->addr, evt->rssi);
	if (!result) {
		return;
	}

	priv = (struct discovery_priv *)&result->_priv;
	priv->pscan_rep_mode = evt->pscan_rep_mode;
	priv->clock_offset = evt->clock_offset;

	result->rssi = evt->rssi;
	memcpy(result->cod, evt->cod, 3);
	memcpy(result->eir, evt->eir, sizeof(result->eir));
}

static void remote_name_request_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_name_req_complete *evt = (void *)buf->data;
	struct bt_br_discovery_result *result;
	struct discovery_priv *priv;
	int eir_len = 240;
	u8_t *eir;
	int i;

	result = get_result_slot(&evt->bdaddr, 0xff);
	if (!result) {
		return;
	}

	priv = (struct discovery_priv *)&result->_priv;
	priv->resolving = 0;

	if (evt->status) {
		goto check_names;
	}

	eir = result->eir;

	while (eir_len) {
		if (eir_len < 2) {
			break;
		};

		/* Look for early termination */
		if (!eir[0]) {
			size_t name_len;

			eir_len -= 2;

			/* name is null terminated */
			name_len = strlen((const char *)evt->name);

			if (name_len > eir_len) {
				eir[0] = eir_len + 1;
				eir[1] = EIR_SHORT_NAME;
			} else {
				eir[0] = name_len + 1;
				eir[1] = EIR_SHORT_NAME;
			}

			memcpy(&eir[2], evt->name, eir[0] - 1);

			break;
		}

		/* Check if field length is correct */
		if (eir[0] > eir_len - 1) {
			break;
		}

		/* next EIR Structure */
		eir_len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

check_names:
	/* if still waiting for names */
	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (priv->resolving) {
			return;
		}
	}

	/* all names resolved, report discovery results */
	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb(discovery_results, discovery_results_count);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
}

static void link_encr(const u16_t handle)
{
	struct bt_hci_cp_set_conn_encrypt *encr;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_CONN_ENCRYPT, sizeof(*encr));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	encr = net_buf_add(buf, sizeof(*encr));
	encr->handle = sys_cpu_to_le16(handle);
	encr->encrypt = 0x01;

	bt_hci_cmd_send_sync(BT_HCI_OP_SET_CONN_ENCRYPT, buf, NULL);
}

static void auth_complete(struct net_buf *buf)
{
	struct bt_hci_evt_auth_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (evt->status) {
		if (conn->state == BT_CONN_CONNECTED) {
			/*
			 * Inform layers above HCI about non-zero authentication
			 * status to make them able cleanup pending jobs.
			 */
			bt_l2cap_encrypt_change(conn, evt->status);
		}
		reset_pairing(conn);
	} else {
		link_encr(handle);
	}

	bt_conn_unref(conn);
}

static void read_remote_features_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_features *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_hci_cp_read_remote_ext_features *cp;
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (evt->status) {
		goto done;
	}

	memcpy(conn->br.features[0], evt->features, sizeof(evt->features));

	if (!BT_FEAT_EXT_FEATURES(conn->br.features)) {
		goto done;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_REMOTE_EXT_FEATURES,
				sizeof(*cp));
	if (!buf) {
		goto done;
	}

	/* Read remote host features (page 1) */
	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = evt->handle;
	cp->page = 0x01;

	bt_hci_cmd_send_sync(BT_HCI_OP_READ_REMOTE_EXT_FEATURES, buf, NULL);

done:
	bt_conn_unref(conn);
}

static void read_remote_ext_features_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_ext_features *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (!evt->status && evt->page == 0x01) {
		memcpy(conn->br.features[1], evt->features,
			   sizeof(conn->br.features[1]));
	}

	bt_conn_unref(conn);
}

static void role_change(struct net_buf *buf)
{
	struct bt_hci_evt_role_change *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("status %u role %u addr %s", evt->status, evt->role,
		   bt_addr_str(&evt->bdaddr));

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->role) {
		conn->role = BT_CONN_ROLE_SLAVE;
	} else {
		conn->role = BT_CONN_ROLE_MASTER;
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_BREDR */

#if defined(CONFIG_BT_SMP)
static int le_set_privacy_mode(const bt_addr_le_t *addr, u8_t mode)
{
	/* Check if set privacy mode command is supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 39, 2)) {
		BT_WARN("Set privacy mode command is not supported");
		return 0;
	}

	BT_DBG("addr %s mode 0x%02x", bt_addr_le_str(addr), mode);

	return hci_api_le_set_privacy_mode(addr->type, (uint8_t *)addr->a.val, mode);
}

static int addr_res_enable(u8_t enable)
{
	BT_DBG("%s", enable ? "enabled" : "disabled");

	return hci_api_le_set_addr_res_enable(enable);
}

static int hci_id_add(const bt_addr_le_t *addr, u8_t val[16])
{
	u8_t *local_irk;
	BT_DBG("addr %s", bt_addr_le_str(addr));

#if defined(CONFIG_BT_PRIVACY)
	local_irk = bt_dev.irk;
#else
	local_irk = NULL;
#endif

	return hci_api_le_add_dev_to_rl(addr->type, (uint8_t *)addr->a.val, val, local_irk);
}

void bt_id_add(struct bt_keys *keys)
{
	bool adv_enabled;
#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled;
#endif /* CONFIG_BT_OBSERVER */
	struct bt_conn *conn;
	int err;

	BT_DBG("addr %s", bt_addr_le_str(&keys->addr));

#if defined(CONFIG_CHIP_CH6121)
	extern int ll_rl_add(uint8_t type,
                uint8_t peer_id_addr[6],
                uint8_t peer_irk[16],
                uint8_t local_irk[16]);
	ll_rl_add(keys->addr.type, keys->addr.a.val, keys->irk.val, NULL);
#endif
	/* Nothing to be done if host-side resolving is used */
	if (!bt_dev.le.rl_size || bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries++;
		return;
	}

	conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
	if (conn) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ID_PENDING);
		keys->flags |= BT_KEYS_ID_PENDING_ADD;
		bt_conn_unref(conn);
		return;
	}

	adv_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	if (adv_enabled) {
		set_advertise_enable(false);
	}

#if defined(CONFIG_BT_OBSERVER)
	scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	/* If there are any existing entries address resolution will be on */
	if (bt_dev.le.rl_entries) {
		err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
		if (err) {
			BT_WARN("Failed to disable address resolution");
			goto done;
		}
	}

	if (bt_dev.le.rl_entries == bt_dev.le.rl_size) {
		BT_WARN("Resolving list size exceeded. Switching to host.");

		err = hci_api_le_clear_rl();
		if (err) {
			BT_ERR("Failed to clear resolution list");
			goto done;
		}

		bt_dev.le.rl_entries++;

		goto done;
	}

	err = hci_id_add(&keys->addr, keys->irk.val);
	if (err) {
		BT_ERR("Failed to add IRK to controller");
		goto done;
	}

	bt_dev.le.rl_entries++;

	/*
	 * According to Core Spec. 5.0 Vol 1, Part A 5.4.5 Privacy Feature
	 *
	 * By default, network privacy mode is used when private addresses are
	 * resolved and generated by the Controller, so advertising packets from
	 * peer devices that contain private addresses will only be accepted.
	 * By changing to the device privacy mode device is only concerned about
	 * its privacy and will accept advertising packets from peer devices
	 * that contain their identity address as well as ones that contain
	 * a private address, even if the peer device has distributed its IRK in
	 * the past.
	 */
	err = le_set_privacy_mode(&keys->addr, BT_HCI_LE_PRIVACY_MODE_DEVICE);
	if (err) {
		BT_ERR("Failed to set privacy mode");
		goto done;
	}

done:
	addr_res_enable(BT_HCI_ADDR_RES_ENABLE);

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (adv_enabled) {
		set_advertise_enable(true);
	}
}

static void keys_add_id(struct bt_keys *keys, void *data)
{
	hci_id_add(&keys->addr, keys->irk.val);
}

void bt_id_del(struct bt_keys *keys)
{
	bool adv_enabled;
#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled;
#endif /* CONFIG_BT_OBSERVER */
	struct bt_conn *conn;
	int err;

	BT_DBG("addr %s", bt_addr_le_str(&keys->addr));
#if defined(CONFIG_CHIP_CH6121)
    int ll_rl_remove(uint8_t type,
            uint8_t peer_id_addr[6]);
	ll_rl_remove(keys->addr.type, keys->addr.a.val);
#endif
	if (!bt_dev.le.rl_size ||
		bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) {
		bt_dev.le.rl_entries--;
		return;
	}

	conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
	if (conn) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ID_PENDING);
		keys->flags |= BT_KEYS_ID_PENDING_DEL;
		bt_conn_unref(conn);
		return;
	}

	adv_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	if (adv_enabled) {
		set_advertise_enable(false);
	}

#if defined(CONFIG_BT_OBSERVER)
	scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
	if (err) {
		BT_ERR("Disabling address resolution failed (err %d)", err);
		goto done;
	}

	/* We checked size + 1 earlier, so here we know we can fit again */
	if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries--;
		keys->keys &= ~BT_KEYS_IRK;
		bt_keys_foreach(BT_KEYS_IRK, keys_add_id, NULL);
		goto done;
	}

	err = hci_api_le_remove_dev_from_rl(keys->addr.type, keys->addr.a.val);
	if (err) {
		BT_ERR("Failed to remove IRK from controller");
		goto done;
	}

	bt_dev.le.rl_entries--;

done:
	/* Only re-enable if there are entries to do resolving with */
	if (bt_dev.le.rl_entries) {
		addr_res_enable(BT_HCI_ADDR_RES_ENABLE);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (adv_enabled) {
		set_advertise_enable(true);
	}
}

static void update_sec_level(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	if (conn->le.keys && (conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		if (conn->le.keys->keys & BT_KEYS_LTK_P256) {
			conn->sec_level = BT_SECURITY_FIPS;
		} else {
			conn->sec_level = BT_SECURITY_HIGH;
		}
	} else {
		conn->sec_level = BT_SECURITY_MEDIUM;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void hci_encrypt_change(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u encrypt 0x%02x", evt->status, handle,
		   evt->encrypt);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		/* TODO report error */
		if (conn->type == BT_CONN_TYPE_LE) {
			/* reset required security level in case of error */
			conn->required_sec_level = conn->sec_level;
#if defined(CONFIG_BT_BREDR)
		} else {
			bt_l2cap_encrypt_change(conn, evt->status);
			reset_pairing(conn);
#endif /* CONFIG_BT_BREDR */
		}
		bt_conn_unref(conn);
		return;
	}

	conn->encrypt = evt->encrypt;

#if defined(CONFIG_BT_SMP)
	if (conn->type == BT_CONN_TYPE_LE) {
		/*
		 * we update keys properties only on successful encryption to
		 * avoid losing valid keys if encryption was not successful.
		 *
		 * Update keys with last pairing info for proper sec level
		 * update. This is done only for LE transport, for BR/EDR keys
		 * are updated on HCI 'Link Key Notification Event'
		 */
		if (conn->encrypt) {
			bt_smp_update_keys(conn);
		}
		update_sec_level(conn);
	}
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		update_sec_level_br(conn);

		if (IS_ENABLED(CONFIG_BT_SMP)) {
			/*
			 * Start SMP over BR/EDR if we are pairing and are
			 * master on the link
			 */
			if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING) &&
				conn->role == BT_CONN_ROLE_MASTER) {
				bt_smp_br_send_pairing_req(conn);
			}
		}

		reset_pairing(conn);
	}
#endif /* CONFIG_BT_BREDR */

	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn);

	bt_conn_unref(conn);
}

static void hci_encrypt_key_refresh_complete(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		bt_l2cap_encrypt_change(conn, evt->status);
		return;
	}

	/*
	 * Update keys with last pairing info for proper sec level update.
	 * This is done only for LE transport. For BR/EDR transport keys are
	 * updated on HCI 'Link Key Notification Event', therefore update here
	 * only security level based on available keys and encryption state.
	 */
#if defined(CONFIG_BT_SMP)
	if (conn->type == BT_CONN_TYPE_LE) {
		bt_smp_update_keys(conn);
		update_sec_level(conn);
	}
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		update_sec_level_br(conn);
	}
#endif /* CONFIG_BT_BREDR */

	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn);
	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_SMP)
static void le_ltk_neg_reply(u16_t handle)
{
	hci_api_le_enctypt_ltk_req_neg_reply(handle);
}

static void le_ltk_request(struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;
	u8_t tk[16];
	u8_t ltk[16];

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("handle %u", handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	/*
	 * if TK is present use it, that means pairing is in progress and
	 * we should use new TK for encryption
	 *
	 * Both legacy STK and LE SC LTK have rand and ediv equal to zero.
	 */
	if (evt->rand == 0 && evt->ediv == 0 && bt_smp_get_tk(conn, tk)) {
		memcpy(ltk, tk, sizeof(ltk));
		hci_api_le_enctypt_ltk_req_reply(handle, ltk);
		goto done;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256, conn->id,
						 &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_SLAVE_LTK,
							 conn->id, &conn->le.dst);
		}
	}

	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_LTK_P256) &&
		evt->rand == 0 && evt->ediv == 0) {

		/* use only enc_size bytes of key for encryption */
		memcpy(ltk, conn->le.keys->ltk.val,
			   conn->le.keys->enc_size);
		if (conn->le.keys->enc_size < sizeof(ltk)) {
			memset(ltk + conn->le.keys->enc_size, 0,
				   sizeof(ltk) - conn->le.keys->enc_size);
		}

		hci_api_le_enctypt_ltk_req_reply(handle, ltk);
		goto done;
	}

#if !defined(CONFIG_BT_SMP_SC_ONLY)
	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_SLAVE_LTK) &&
		!memcmp(conn->le.keys->slave_ltk.rand, &evt->rand, 8) &&
		!memcmp(conn->le.keys->slave_ltk.ediv, &evt->ediv, 2)) {

		/* use only enc_size bytes of key for encryption */
		memcpy(ltk, conn->le.keys->slave_ltk.val,
			   conn->le.keys->enc_size);
		if (conn->le.keys->enc_size < sizeof(ltk)) {
			memset(ltk + conn->le.keys->enc_size, 0,
				   sizeof(ltk) - conn->le.keys->enc_size);
		}

		hci_api_le_enctypt_ltk_req_reply(handle, ltk);
		goto done;
	}
#endif /* !CONFIG_BT_SMP_SC_ONLY */

	le_ltk_neg_reply(evt->handle);
	extern void key_miss_cb();
	key_miss_cb();
	BT_ERR("Local key miss please unbound the device");

done:
	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_SMP */

int hci_api_le_event_pkey_complete(	u8_t status, u8_t key[64])
{
	struct bt_pub_key_cb *cb;
	BT_DBG("status: 0x%x", status);

	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	if (!status) {
		memcpy(pub_key, key, 64);
		atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
		cb->func(status ? NULL : key);
	}

	return 0;
}

static void le_pkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt = (void *)buf->data;

	hci_api_le_event_pkey_complete(evt->status, evt->key);
}

int hci_api_le_event_dhkey_complete(	u8_t status, u8_t dhkey[32])
{
	BT_DBG("status: 0x%x", status);

	if (dh_key_cb) {
		dh_key_cb(status ? NULL : dhkey);
		dh_key_cb = NULL;
	}

	return 0;
}

static void le_dhkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt = (void *)buf->data;

	hci_api_le_event_dhkey_complete(evt->status, evt->dhkey);
}

static void hci_reset_complete()
{
	atomic_t flags;

	scan_dev_found_cb = NULL;
#if defined(CONFIG_BT_BREDR)
	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
#endif /* CONFIG_BT_BREDR */

	flags = (atomic_get(bt_dev.flags) & BT_DEV_PERSISTENT_FLAGS);
	atomic_set(bt_dev.flags, flags);
}

#if defined(CONFIG_BT_OBSERVER)
static int start_le_scan(u8_t scan_type, u16_t interval, u16_t window)
{
	u8_t  addr_type;
	int err;

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		err = le_set_private_addr(BT_ID_DEFAULT);
		if (err) {
			return err;
		}

		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			addr_type = BT_ADDR_LE_RANDOM;
		}
	} else {
		addr_type =  bt_dev.id_addr[0].type;

		/* Use NRPA unless identity has been explicitly requested
		 * (through Kconfig), or if there is no advertising ongoing.
		 */
		if (!IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
			scan_type == BT_HCI_LE_SCAN_ACTIVE &&
			!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
			err = le_set_private_addr(BT_ID_DEFAULT);
			if (err) {
				return err;
			}

			addr_type = BT_ADDR_LE_RANDOM;
		}
	}

	err = hci_api_le_scan_param_set(scan_type, interval, window, addr_type, atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_FILTER));
	if (err)
	{
		return err;
	}

	err = set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	if (err) {
		return err;
	}

	if (scan_type == BT_HCI_LE_SCAN_ACTIVE) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN);
	}

	return 0;
}

int bt_le_scan_update(bool fast_scan)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return 0;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		u16_t interval, window;
		struct bt_conn *conn;

		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT_SCAN);
		if (!conn) {
			return 0;
		}

		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);

		bt_conn_unref(conn);

		if (fast_scan) {
			interval = BT_GAP_SCAN_FAST_INTERVAL;
			window = BT_GAP_SCAN_FAST_WINDOW;
		} else {
			interval = BT_GAP_SCAN_SLOW_INTERVAL_1;
			window = BT_GAP_SCAN_SLOW_WINDOW_1;
		}

		return start_le_scan(BT_HCI_LE_SCAN_PASSIVE, interval, window);
	}

	return 0;
}

void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data)
{
	while (ad->len > 1) {
		struct bt_data data;
		u8_t len;

		len = net_buf_simple_pull_u8(ad);
		if (len == 0) {
			/* Early termination */
			return;
		}

		if (len > ad->len) {
			BT_WARN("Malformed data");
			return;
		}

		data.type = net_buf_simple_pull_u8(ad);
		data.data_len = len - 1;
		data.data = ad->data;

		if (!func(&data, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

void le_adv_report(struct net_buf *buf)
{
	u8_t num_reports = net_buf_pull_u8(buf);
	struct bt_hci_evt_le_advertising_info *info;

	BT_DBG("Adv number of reports %u",  num_reports);

	while (num_reports--) {
		bt_addr_le_t id_addr;
		s8_t rssi;

		info = (void *)buf->data;
		net_buf_pull(buf, sizeof(*info));

		rssi = info->data[info->length];

		BT_DBG("%s event %u, len %u, rssi %d dBm",
			   bt_addr_le_str(&info->addr),
			   info->evt_type, info->length, rssi);

		if (info->addr.type == BT_ADDR_LE_PUBLIC_ID ||
			info->addr.type == BT_ADDR_LE_RANDOM_ID) {
			bt_addr_le_copy(&id_addr, &info->addr);
			id_addr.type -= BT_ADDR_LE_PUBLIC_ID;
		} else {
			bt_addr_le_copy(&id_addr,
					find_id_addr(bt_dev.adv_id,
							 &info->addr));
		}

		/* bugfix: use scan_dev_found_cb_tmp to avoid jump to NULL */
		bt_le_scan_cb_t *scan_dev_found_cb_tmp = scan_dev_found_cb;

		if (scan_dev_found_cb_tmp) {
			struct net_buf_simple_state state;

			net_buf_simple_save(&buf->b, &state);

			buf->len = info->length;
			scan_dev_found_cb_tmp(&id_addr, rssi, info->evt_type,
					  &buf->b);
			net_buf_simple_restore(&buf->b, &state);
		}

#if defined(CONFIG_BT_CENTRAL)
		check_pending_conn(&id_addr, &info->addr, info->evt_type);
#endif /* CONFIG_BT_CENTRAL */

		/* Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.65.2.
		 */
		net_buf_pull(buf, info->length + sizeof(rssi));
	}
}
#endif /* CONFIG_BT_OBSERVER */

static void hci_le_meta_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt = (void *)buf->data;

	BT_DBG("subevent 0x%02x", evt->subevent);

	net_buf_pull(buf, sizeof(*evt));

	switch (evt->subevent) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_LE_CONN_COMPLETE:
		le_legacy_conn_complete(buf);
		break;
	case BT_HCI_EVT_LE_ENH_CONN_COMPLETE:
		le_enh_conn_complete((void *)buf->data);
		break;
	case BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE:
		le_conn_update_complete(buf);
		break;
	case BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE:
		le_remote_feat_complete(buf);
		break;
	case BT_HCI_EVT_LE_CONN_PARAM_REQ:
		le_conn_param_req(buf);
		break;
	case BT_HCI_EVT_LE_DATA_LEN_CHANGE:
		le_data_len_change(buf);
		break;
	case BT_HCI_EVT_LE_PHY_UPDATE_COMPLETE:
		le_phy_update_complete(buf);
		break;
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_SMP)
	case BT_HCI_EVT_LE_LTK_REQUEST:
		le_ltk_request(buf);
		break;
#endif /* CONFIG_BT_SMP */
	case BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE:
		le_pkey_complete(buf);
		break;
	case BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE:
		le_dhkey_complete(buf);
		break;
#if defined(CONFIG_BT_OBSERVER)
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		le_adv_report(buf);
		break;
#endif /* CONFIG_BT_OBSERVER */
	default:
		BT_WARN("Unhandled LE event 0x%02x len %u: %s",
			evt->subevent, buf->len, bt_hex(buf->data, buf->len));
		break;
	}
}

void hci_event(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	BT_DBG("event 0x%02x", hdr->evt);

	BT_ASSERT(!bt_hci_evt_is_prio(hdr->evt));

	net_buf_pull(buf, sizeof(*hdr));

	switch (hdr->evt) {
#if defined(CONFIG_BT_BREDR)
	case BT_HCI_EVT_CONN_REQUEST:
		conn_req(buf);
		break;
	case BT_HCI_EVT_CONN_COMPLETE:
		conn_complete(buf);
		break;
	case BT_HCI_EVT_PIN_CODE_REQ:
		pin_code_req(buf);
		break;
	case BT_HCI_EVT_LINK_KEY_NOTIFY:
		link_key_notify(buf);
		break;
	case BT_HCI_EVT_LINK_KEY_REQ:
		link_key_req(buf);
		break;
	case BT_HCI_EVT_IO_CAPA_RESP:
		io_capa_resp(buf);
		break;
	case BT_HCI_EVT_IO_CAPA_REQ:
		io_capa_req(buf);
		break;
	case BT_HCI_EVT_SSP_COMPLETE:
		ssp_complete(buf);
		break;
	case BT_HCI_EVT_USER_CONFIRM_REQ:
		user_confirm_req(buf);
		break;
	case BT_HCI_EVT_USER_PASSKEY_NOTIFY:
		user_passkey_notify(buf);
		break;
	case BT_HCI_EVT_USER_PASSKEY_REQ:
		user_passkey_req(buf);
		break;
	case BT_HCI_EVT_INQUIRY_COMPLETE:
		inquiry_complete(buf);
		break;
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
		inquiry_result_with_rssi(buf);
		break;
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		extended_inquiry_result(buf);
		break;
	case BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE:
		remote_name_request_complete(buf);
		break;
	case BT_HCI_EVT_AUTH_COMPLETE:
		auth_complete(buf);
		break;
	case BT_HCI_EVT_REMOTE_FEATURES:
		read_remote_features_complete(buf);
		break;
	case BT_HCI_EVT_REMOTE_EXT_FEATURES:
		read_remote_ext_features_complete(buf);
		break;
	case BT_HCI_EVT_ROLE_CHANGE:
		role_change(buf);
		break;
	case BT_HCI_EVT_SYNC_CONN_COMPLETE:
		synchronous_conn_complete(buf);
		break;
#endif
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_DISCONN_COMPLETE:
		hci_disconn_complete(buf);
		break;
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	case BT_HCI_EVT_ENCRYPT_CHANGE:
		hci_encrypt_change(buf);
		break;
	case BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE:
		hci_encrypt_key_refresh_complete(buf);
		break;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */
	case BT_HCI_EVT_LE_META_EVENT:
		hci_le_meta_event(buf);
		break;
	default:
		BT_WARN("Unhandled event 0x%02x len %u: %s", hdr->evt,
			buf->len, bt_hex(buf->data, buf->len));
		break;

	}

	net_buf_unref(buf);
}


#if defined(CONFIG_BT_BREDR)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	u16_t pkts;

	BT_DBG("status %u", rp->status);

	bt_dev.br.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.br.mtu);

	k_sem_init(&bt_dev.br.pkts, pkts, pkts);
}
#elif defined(CONFIG_BT_CONN)
static void read_buffer_size_complete(u16_t acl_max_len, uint8_t  sco_max_len, u16_t acl_max_num, u16_t sco_max_num)
{
	u16_t pkts;

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le.mtu) {
		return;
	}

	bt_dev.le.mtu = acl_max_len;
	pkts = acl_max_num;

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.le.mtu);

	pkts = min(pkts, CONFIG_BT_CONN_TX_MAX);

	k_sem_init(&bt_dev.le.pkts, pkts, pkts);
}
#endif

static int common_init(void)
{
	int err;

	if (!(bt_dev.drv->quirks & BT_QUIRK_NO_RESET)) {
		err = hci_api_reset();
		if (err)
		{
			return err;
		}
		hci_reset_complete();
	}

	if (bt_dev.id_count > 0 && bt_dev.id_addr[0].type == BT_ADDR_LE_PUBLIC)
	{
		hci_api_le_set_bdaddr(bt_dev.id_addr[0].a.val);
		bt_dev.id_count = 0;
		memset(bt_dev.id_addr, 0, sizeof(bt_dev.id_addr));
	}
	else if (bt_dev.id_count > 0 && bt_dev.id_addr[0].type == BT_ADDR_LE_RANDOM)
	{
		set_random_address(&bt_dev.id_addr[0].a);
	}

	/* Read Local Supported Features */
	err = hci_api_read_local_feature(bt_dev.features[0]);
	if (err) {
		return err;
	}

	/* Read Local Version Information */
	err = hci_api_read_local_version_info(&bt_dev.hci_version, 
											&bt_dev.lmp_version, 
											&bt_dev.hci_revision,
											&bt_dev.lmp_subversion,
											&bt_dev.manufacturer);
	if (err) {
		return err;
	}

	/* Read Bluetooth Address */
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR)) {
		bt_addr_t bd_addr = {0};
		err = hci_api_read_bdaddr(bd_addr.val);
		if (!err)
		{
			if (!bt_addr_cmp(&bd_addr, BT_ADDR_ANY) ||
				!bt_addr_cmp(&bd_addr, BT_ADDR_NONE)) {
				err = -ENODATA;
			}
		}
		if (!err) {
			memcpy(bt_dev.id_addr[0].a.val, bd_addr.val, 6);
			bt_dev.id_addr[0].type = BT_ADDR_LE_PUBLIC;
			bt_dev.id_count = 1;
		}
	}

	/* Read Local Supported Commands */
	err = hci_api_read_local_support_command(bt_dev.supported_commands);
	if (err) {
		return err;
	}

	/*
	 * Report "LE Read Local P-256 Public Key" and "LE Generate DH Key" as
	 * supported if TinyCrypt ECC is used for emulation.
	 */
	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_dev.supported_commands[34] |= 0x02;
		bt_dev.supported_commands[34] |= 0x04;
	}

	if (IS_ENABLED(CONFIG_BT_HOST_CRYPTO)) {
		/* Initialize the PRNG so that it is safe to use it later
		 * on in the initialization process.
		 */
		err = prng_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	err = set_flow_control();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CONN */

	return 0;
}

static int le_set_event_mask(void)
{
	u64_t mask = 0;

	mask |= BT_EVT_MASK_LE_ADVERTISING_REPORT;

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_ENH_CONN_COMPLETE;
		} else {
			mask |= BT_EVT_MASK_LE_CONN_COMPLETE;
		}

		mask |= BT_EVT_MASK_LE_CONN_UPDATE_COMPLETE;
		mask |= BT_EVT_MASK_LE_REMOTE_FEAT_COMPLETE;

		if (BT_FEAT_LE_CONN_PARAM_REQ_PROC(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_CONN_PARAM_REQ;
		}

		if (BT_FEAT_LE_DLE(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_DATA_LEN_CHANGE;
		}

		if (BT_FEAT_LE_PHY_2M(bt_dev.le.features) ||
			BT_FEAT_LE_PHY_CODED(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_PHY_UPDATE_COMPLETE;
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
		BT_FEAT_LE_ENCR(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_LE_LTK_REQUEST;
	}

	/*
	 * If "LE Read Local P-256 Public Key" and "LE Generate DH Key" are
	 * supported we need to enable events generated by those commands.
	 */
	if ((bt_dev.supported_commands[34] & 0x02) &&
		(bt_dev.supported_commands[34] & 0x04)) {
		mask |= BT_EVT_MASK_LE_P256_PUBLIC_KEY_COMPLETE;
		mask |= BT_EVT_MASK_LE_GENERATE_DHKEY_COMPLETE;
	}

	return hci_api_le_set_event_mask(mask);
}

static int le_init(void)
{
	int err;
	/* For now we only support LE capable controllers */
	if (!BT_FEAT_LE(bt_dev.features)) {
		BT_ERR("Non-LE capable controller detected!");
		return -ENODEV;
	}

	/* Read Low Energy Supported Features */
	err = hci_api_le_read_local_feature(bt_dev.le.features);
	if (err) {
		return err;
	}


#if defined(CONFIG_BT_CONN)
	u8_t le_max_num;
	/* Read LE Buffer Size */
	err = hci_api_le_read_buffer_size_complete(&bt_dev.le.mtu,&le_max_num);
	if (err) {
		return err;
	}
	if (bt_dev.le.mtu)
	{
		bt_dev.le.mtu_init = bt_dev.le.mtu;
		le_max_num = min(le_max_num, CONFIG_BT_CONN_TX_MAX);
		k_sem_init(&bt_dev.le.pkts, le_max_num, le_max_num);
	}
#endif

	if (BT_FEAT_BREDR(bt_dev.features)) {
		/* Explicitly enable LE for dual-mode controllers */
		err = hci_api_le_write_host_supp(0x01, 0x00);
		if (err) {
			return err;
		}
	}

	/* Read LE Supported States */
	if (BT_CMD_LE_STATES(bt_dev.supported_commands)) {
		err = hci_api_le_read_support_states(&bt_dev.le.states);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CONN) &&
		BT_FEAT_LE_DLE(bt_dev.le.features)) {
		u16_t tx_octets, tx_time;

		err = hci_api_le_get_max_data_len(&tx_octets, &tx_time);
		if (err) {
			return err;
		}

		err = hci_api_le_set_default_data_len(tx_octets, tx_time);
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_SMP)
	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {

		err = hci_api_le_read_rl_size(&bt_dev.le.rl_size);
		if (err) {
			return err;
		}

		BT_DBG("Resolving List size %u", bt_dev.le.rl_size);
	}
#endif

	return  le_set_event_mask();
}

#if defined(CONFIG_BT_BREDR)
static int read_ext_features(void)
{
	int i;

	/* Read Local Supported Extended Features */
	for (i = 1; i < LMP_FEAT_PAGES_COUNT; i++) {
		struct bt_hci_cp_read_local_ext_features *cp;
		struct bt_hci_rp_read_local_ext_features *rp;
		struct net_buf *buf, *rsp;
		int err;

		buf = bt_hci_cmd_create(BT_HCI_OP_READ_LOCAL_EXT_FEATURES,
					sizeof(*cp));
		if (!buf) {
			return -ENOBUFS;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->page = i;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_EXT_FEATURES,
					   buf, &rsp);
		if (err) {
			return err;
		}

		rp = (void *)rsp->data;

		memcpy(&bt_dev.features[i], rp->ext_features,
			   sizeof(bt_dev.features[i]));

		if (rp->max_page <= i) {
			net_buf_unref(rsp);
			break;
		}

		net_buf_unref(rsp);
	}

	return 0;
}

void device_supported_pkt_type(void)
{
	/* Device supported features and sco packet types */
	if (BT_FEAT_HV2_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_HV2);
	}

	if (BT_FEAT_HV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_HV3);
	}

	if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV3);
	}

	if (BT_FEAT_EV4_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV4);
	}

	if (BT_FEAT_EV5_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV5);
	}

	if (BT_FEAT_2EV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_2EV3);
	}

	if (BT_FEAT_3EV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_3EV3);
	}

	if (BT_FEAT_3SLOT_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_2EV5 |
						HCI_PKT_TYPE_ESCO_3EV5);
	}
}

static int br_init(void)
{
	struct net_buf *buf;
	struct bt_hci_cp_write_ssp_mode *ssp_cp;
	struct bt_hci_cp_write_inquiry_mode *inq_cp;
	struct bt_hci_write_local_name *name_cp;
	int err;

	/* Read extended local features */
	if (BT_FEAT_EXT_FEATURES(bt_dev.features)) {
		err = read_ext_features();
		if (err) {
			return err;
		}
	}

	/* Add local supported packet types to bt_dev */
	device_supported_pkt_type();

	/* Get BR/EDR buffer size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BUFFER_SIZE, NULL, &buf);
	if (err) {
		return err;
	}

	read_buffer_size_complete(buf);
	net_buf_unref(buf);

	/* Set SSP mode */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SSP_MODE, sizeof(*ssp_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	ssp_cp = net_buf_add(buf, sizeof(*ssp_cp));
	ssp_cp->mode = 0x01;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SSP_MODE, buf, NULL);
	if (err) {
		return err;
	}

	/* Enable Inquiry results with RSSI or extended Inquiry */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_INQUIRY_MODE, sizeof(*inq_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	inq_cp = net_buf_add(buf, sizeof(*inq_cp));
	inq_cp->mode = 0x02;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_INQUIRY_MODE, buf, NULL);
	if (err) {
		return err;
	}

	/* Set local name */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_LOCAL_NAME, sizeof(*name_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	name_cp = net_buf_add(buf, sizeof(*name_cp));
	strncpy((char *)name_cp->local_name, CONFIG_BT_DEVICE_NAME,
		sizeof(name_cp->local_name));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_LOCAL_NAME, buf, NULL);
	if (err) {
		return err;
	}

	/* Set page timeout*/
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_PAGE_TIMEOUT, sizeof(u16_t));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_le16(buf, CONFIG_BT_PAGE_TIMEOUT);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_PAGE_TIMEOUT, buf, NULL);
	if (err) {
		return err;
	}

	/* Enable BR/EDR SC if supported */
	if (BT_FEAT_SC(bt_dev.features)) {
		struct bt_hci_cp_write_sc_host_supp *sc_cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SC_HOST_SUPP,
					sizeof(*sc_cp));
		if (!buf) {
			return -ENOBUFS;
		}

		sc_cp = net_buf_add(buf, sizeof(*sc_cp));
		sc_cp->sc_support = 0x01;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SC_HOST_SUPP, buf,
					   NULL);
		if (err) {
			return err;
		}
	}

	return 0;
}
#else
static int br_init(void)
{
#if defined(CONFIG_BT_CONN)
	int err;
	u16_t acl_max_len = 0;
	u8_t  sco_max_len = 0;
	u16_t acl_max_num = 0;
	u16_t sco_max_num = 0;

	if (bt_dev.le.mtu) {
		return 0;
	}

	/* Use BR/EDR buffer size if LE reports zero buffers */
	err = hci_api_read_buffer_size(&acl_max_len, &sco_max_len, &acl_max_num, &sco_max_num);
	if (err) {
		return err;
	}

	read_buffer_size_complete(acl_max_len, sco_max_len, acl_max_num, sco_max_num);
#endif /* CONFIG_BT_CONN */

	return 0;
}
#endif

static int set_event_mask(void)
{
	u64_t mask = 0;

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		/* Since we require LE support, we can count on a
		 * Bluetooth 4.0 feature set
		 */
		mask |= BT_EVT_MASK_INQUIRY_COMPLETE;
		mask |= BT_EVT_MASK_CONN_COMPLETE;
		mask |= BT_EVT_MASK_CONN_REQUEST;
		mask |= BT_EVT_MASK_AUTH_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_NAME_REQ_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_FEATURES;
		mask |= BT_EVT_MASK_ROLE_CHANGE;
		mask |= BT_EVT_MASK_PIN_CODE_REQ;
		mask |= BT_EVT_MASK_LINK_KEY_REQ;
		mask |= BT_EVT_MASK_LINK_KEY_NOTIFY;
		mask |= BT_EVT_MASK_INQUIRY_RESULT_WITH_RSSI;
		mask |= BT_EVT_MASK_REMOTE_EXT_FEATURES;
		mask |= BT_EVT_MASK_SYNC_CONN_COMPLETE;
		mask |= BT_EVT_MASK_EXTENDED_INQUIRY_RESULT;
		mask |= BT_EVT_MASK_IO_CAPA_REQ;
		mask |= BT_EVT_MASK_IO_CAPA_RESP;
		mask |= BT_EVT_MASK_USER_CONFIRM_REQ;
		mask |= BT_EVT_MASK_USER_PASSKEY_REQ;
		mask |= BT_EVT_MASK_SSP_COMPLETE;
		mask |= BT_EVT_MASK_USER_PASSKEY_NOTIFY;
	}

	mask |= BT_EVT_MASK_HARDWARE_ERROR;
	mask |= BT_EVT_MASK_DATA_BUFFER_OVERFLOW;
	mask |= BT_EVT_MASK_LE_META_EVENT;

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		mask |= BT_EVT_MASK_DISCONN_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_VERSION_INFO;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
		BT_FEAT_LE_ENCR(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_ENCRYPT_CHANGE;
		mask |= BT_EVT_MASK_ENCRYPT_KEY_REFRESH_COMPLETE;
	}

	return hci_api_set_event_mask(mask);
}

static inline int create_random_addr(bt_addr_le_t *addr)
{
	addr->type = BT_ADDR_LE_RANDOM;

	return bt_rand(addr->a.val, 6);
}

int bt_addr_le_create_nrpa(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_NRPA(&addr->a);

	return 0;
}

int bt_addr_le_create_static(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_STATIC(&addr->a);

	return 0;
}

int bt_setup_id_addr(void)
{
	int err;
#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Check for VS_Read_Static_Addresses support. Only read the
	 * addresses if the user has not already configured one or
	 * more identities (!bt_dev.id_count).
	 */
	if (!bt_dev.id_count && (bt_dev.vs_commands[1] & BIT(0))) {
		struct bt_hci_rp_vs_read_static_addrs *rp;
		struct net_buf *rsp;
		int err, i;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS,
					   NULL, &rsp);
		if (err) {
			BT_WARN("Failed to read static addresses");
			goto generate;
		}

		rp = (void *)rsp->data;
		bt_dev.id_count = min(rp->num_addrs, CONFIG_BT_ID_MAX);
		for (i = 0; i < bt_dev.id_count; i++) {
			bt_dev.id_addr[i].type = BT_ADDR_LE_RANDOM;
			bt_addr_copy(&bt_dev.id_addr[i].a, &rp->a[i].bdaddr);
		}

		net_buf_unref(rsp);

		if (bt_dev.id_count) {
			return set_random_address(&bt_dev.id_addr[0].a);
		}

		BT_WARN("No static addresses stored in controller");
	} else {
		BT_WARN("Read Static Addresses command not available");
	}

generate:
#endif

	err = bt_id_create(NULL, NULL);
	if (bt_dev.id_count) {
    	return set_random_address(&bt_dev.id_addr[0].a);
	}
	return err;
}

#if defined(CONFIG_BT_DEBUG)
const char *ver_str(u8_t ver)
{
	const char * const str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0",
	};

	if (ver < ARRAY_SIZE(str)) {
		return str[ver];
	}

	return "unknown";
}

void bt_dev_show_info(void)
{
	int i;

	BT_INFO("Identity%s: %s", bt_dev.id_count > 1 ? "[0]" : "",
		bt_addr_le_str(&bt_dev.id_addr[0]));

	for (i = 1; i < bt_dev.id_count; i++) {
		BT_INFO("Identity[%d]: %s",
			i, bt_addr_le_str(&bt_dev.id_addr[i]));
	}

	BT_INFO("HCI: version %s (0x%02x) revision 0x%04x, manufacturer 0x%04x",
		ver_str(bt_dev.hci_version), bt_dev.hci_version,
		bt_dev.hci_revision, bt_dev.manufacturer);
	BT_INFO("LMP: version %s (0x%02x) subver 0x%04x",
		ver_str(bt_dev.lmp_version), bt_dev.lmp_version,
		bt_dev.lmp_subversion);
}
#else
void bt_dev_show_info(void)
{
}
#endif /* CONFIG_BT_DEBUG */

#if defined(CONFIG_BT_HCI_VS_EXT)
#if defined(CONFIG_BT_DEBUG)
static const char *vs_hw_platform(u16_t platform)
{
	static const char * const plat_str[] = {
		"reserved", "Intel Corporation", "Nordic Semiconductor",
		"NXP Semiconductors" };

	if (platform < ARRAY_SIZE(plat_str)) {
		return plat_str[platform];
	}

	return "unknown";
}

static const char *vs_hw_variant(u16_t platform, u16_t variant)
{
	static const char * const nordic_str[] = {
		"reserved", "nRF51x", "nRF52x"
	};

	if (platform != BT_HCI_VS_HW_PLAT_NORDIC) {
		return "unknown";
	}

	if (variant < ARRAY_SIZE(nordic_str)) {
		return nordic_str[variant];
	}

	return "unknown";
}

static const char *vs_fw_variant(u8_t variant)
{
	static const char * const var_str[] = {
		"Standard Bluetooth controller",
		"Vendor specific controller",
		"Firmware loader",
		"Rescue image",
	};

	if (variant < ARRAY_SIZE(var_str)) {
		return var_str[variant];
	}

	return "unknown";
}
#endif /* CONFIG_BT_DEBUG */

static void hci_vs_init(void)
{
	union {
		struct bt_hci_rp_vs_read_version_info *info;
		struct bt_hci_rp_vs_read_supported_commands *cmds;
		struct bt_hci_rp_vs_read_supported_features *feat;
	} rp;
	struct net_buf *rsp;
	int err;

	/* If heuristics is enabled, try to guess HCI VS support by looking
	 * at the HCI version and identity address. We haven't tried to set
	 * a static random address yet at this point, so the identity will
	 * either be zeroes or a valid public address.
	 */
	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
		(bt_dev.hci_version < BT_HCI_VERSION_5_0 ||
		 (!atomic_test_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR) &&
		  bt_addr_le_cmp(&bt_dev.id_addr[0], BT_ADDR_LE_ANY)))) {
		BT_WARN("Controller doesn't seem to support Zephyr vendor HCI");
		return;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_VERSION_INFO, NULL, &rsp);
	if (err) {
		BT_WARN("Vendor HCI extensions not available");
		return;
	}

#if defined(CONFIG_BT_DEBUG)
	rp.info = (void *)rsp->data;
	BT_INFO("HW Platform: %s (0x%04x)",
		vs_hw_platform(sys_le16_to_cpu(rp.info->hw_platform)),
		sys_le16_to_cpu(rp.info->hw_platform));
	BT_INFO("HW Variant: %s (0x%04x)",
		vs_hw_variant(sys_le16_to_cpu(rp.info->hw_platform),
				  sys_le16_to_cpu(rp.info->hw_variant)),
		sys_le16_to_cpu(rp.info->hw_variant));
	BT_INFO("Firmware: %s (0x%02x) Version %u.%u Build %u",
		vs_fw_variant(rp.info->fw_variant), rp.info->fw_variant,
		rp.info->fw_version, sys_le16_to_cpu(rp.info->fw_revision),
		sys_le32_to_cpu(rp.info->fw_build));
#endif /* CONFIG_BT_DEBUG */

	net_buf_unref(rsp);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS,
				   NULL, &rsp);
	if (err) {
		BT_WARN("Failed to read supported vendor features");
		return;
	}

	rp.cmds = (void *)rsp->data;
	memcpy(bt_dev.vs_commands, rp.cmds->commands, BT_DEV_VS_CMDS_MAX);
	net_buf_unref(rsp);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES,
				   NULL, &rsp);
	if (err) {
		BT_WARN("Failed to read supported vendor commands");
		return;
	}

	rp.feat = (void *)rsp->data;
	memcpy(bt_dev.vs_features, rp.feat->features, BT_DEV_VS_FEAT_MAX);
	net_buf_unref(rsp);
}

#endif /* CONFIG_BT_HCI_VS_EXT */

static int hci_init(void)
{
	int err;

	err = hci_api_init();
	if (err) {
		return err;
	}

	err = common_init();
	if (err) {
		return err;
	}

	err = le_init();
	if (err) {
		return err;
	}

	if (BT_FEAT_BREDR(bt_dev.features)) {
		err = br_init();
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_BREDR)) {
		BT_ERR("Non-BR/EDR controller detected");
		return -EIO;
	}

	err = set_event_mask();
	if (err) {
		return err;
	}

	hci_api_vs_init();

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) && !bt_dev.id_count) {
		BT_DBG("No public address. Trying to set static random.");
		err = bt_setup_id_addr();
		if (err) {
			BT_ERR("Unable to set identity address");
			return err;
		}

		bt_dev_show_info();
	}

	return 0;
}

int bt_send(struct net_buf *buf)
{
	BT_DBG("buf %p len %u type %u", buf, buf->len, bt_buf_get_type(buf));

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

	return bt_dev.drv->send(buf);
}

int bt_recv(struct net_buf *buf)
{
	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	BT_DBG("buf %p len %u", buf, buf->len);

	switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_IN:
#if defined(CONFIG_BT_RECV_IS_RX_THREAD)
		hci_acl(buf);
#else
		net_buf_put(&bt_dev.rx_queue, buf);
#endif
		return 0;
#endif /* BT_CONN */
	case BT_BUF_EVT:
#if defined(CONFIG_BT_RECV_IS_RX_THREAD)
		hci_event(buf);
#else
		net_buf_put(&bt_dev.rx_queue, buf);
#endif
		return 0;
	default:
		BT_ERR("Invalid buf type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}
}

int bt_hci_driver_register(const struct bt_hci_driver *drv)
{
	if (bt_dev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev.drv = drv;

	BT_DBG("Registered %s", drv->name ? drv->name : "");

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, drv->bus,
				 BT_ADDR_ANY, drv->name ? drv->name : "bt0");

	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
static int irk_init(void)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		BT_DBG("Expecting settings to handle local IRK");
	} else {
		int err;

		err = bt_rand(&bt_dev.irk[0], 16);
		if (err) {
			return err;
		}

		BT_WARN("Using temporary IRK");
	}

	return 0;
}
#endif /* CONFIG_BT_PRIVACY */

static int bt_init(void)
{
	int err;

	err = hci_init();
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		err = bt_conn_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	err = irk_init();
	if (err) {
		return err;
	}

	k_delayed_work_init(&bt_dev.rpa_update, rpa_timeout);
#endif

	bt_monitor_send(BT_MONITOR_OPEN_INDEX, NULL, 0);
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		bt_le_scan_update(false);
	}

	if (bt_dev.id_count > 0) {
		atomic_set_bit(bt_dev.flags, BT_DEV_PRESET_ID);
	} else if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		BT_WARN("No ID address. Expecting one to come from storage.");
	}

	return 0;
}

static void init_work(struct k_work *work)
{
	int err;

	err = bt_init();
	if (ready_cb) {
		ready_cb(err);
	}
}

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
static void hci_rx_thread(void * arg)
{
	struct net_buf *buf = NULL;

	BT_DBG("started");

	while (1) {
		//BT_INFO("calling fifo_get_wait");
		csi_vic_enable_irq(8);
		buf = net_buf_get(&bt_dev.rx_queue, K_FOREVER);

		if (bt_buf_get_type(buf) != BT_BUF_EVT)
			BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
			   buf->len);

		switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BT_CONN)
		case BT_BUF_ACL_IN:
			hci_acl(buf);
			break;
#endif /* CONFIG_BT_CONN */
		case BT_BUF_EVT:
			hci_event(buf);
			break;
		default:
			BT_ERR("Unknown buf type %u", bt_buf_get_type(buf));
			net_buf_unref(buf);
			break;
		}

		/* Make sure we don't hog the CPU if the rx_queue never
		 * gets empty.
		 */
		k_yield();
	}
}
#endif /* !CONFIG_BT_RECV_IS_RX_THREAD */

int bt_enable(bt_ready_cb_t cb)
{
	int err;

	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_settings_init();
		if (err) {
			return err;
		}
	} else {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}

	NET_BUF_POOL_INIT(hci_rx_pool);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	NET_BUF_POOL_INIT(acl_in_pool);
#endif

	k_fifo_init(&bt_dev.tx_event);

	k_work_q_start();
	k_work_init(&bt_dev.init, init_work);

	k_sem_init(&g_poll_sem, 0, 1);

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	k_fifo_init(&bt_dev.rx_queue);
#endif

	ready_cb = cb;


#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	/* RX thread */
	k_thread_spawn(rx_thread_data, "hci rx", rx_thread_stack, K_THREAD_STACK_SIZEOF(rx_thread_stack),\
				hci_rx_thread, NULL, CONFIG_BT_RX_PRIO);
#endif

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_hci_ecc_init();
	}

	err = bt_dev.drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	if (!cb) {
		return bt_init();
	}

	k_work_submit(&bt_dev.init);
	return 0;
}

struct bt_ad {
	const struct bt_data *data;
	size_t len;
};

static int set_ad(u16_t hci_op, const struct bt_ad *ad, size_t ad_len)
{
	struct bt_hci_cp_le_set_adv_data set_data;
	int c, i;

	memset(&set_data, 0, sizeof(set_data));

	for (c = 0; c < ad_len; c++) {
		const struct bt_data *data = ad[c].data;

		for (i = 0; i < ad[c].len; i++) {
			int len = data[i].data_len;
			u8_t type = data[i].type;

			/* Check if ad fit in the remaining buffer */
			if (set_data.len + len + 2 > 31) {
				len = 31 - (set_data.len + 2);
				if (type != BT_DATA_NAME_COMPLETE || !len) {
					return -EINVAL;
				}
				type = BT_DATA_NAME_SHORTENED;
			}

			set_data.data[set_data.len++] = len + 1;
			set_data.data[set_data.len++] = type;

			memcpy(&set_data.data[set_data.len], data[i].data,
				   len);
			set_data.len += len;
		}
	}

	if (hci_op == BT_HCI_OP_LE_SET_ADV_DATA)
	{
		return hci_api_le_set_ad_data(set_data.len, set_data.data);
	}
	else if (hci_op == BT_HCI_OP_LE_SET_SCAN_RSP_DATA)
	{
		return hci_api_le_set_sd_data(set_data.len, set_data.data);
	}

	return 0;
}

int bt_set_bdaddr(const bt_addr_le_t *addr)
{
	bt_addr_copy(&bt_dev.id_addr[0].a, &addr->a);
	bt_dev.id_addr[0].type = BT_ADDR_LE_PUBLIC;
	bt_dev.id_count = 1;

	return 0;
}

int bt_set_name(const char *name)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	size_t len = strlen(name);
	int err;

	if (len >= sizeof(bt_dev.name)) {
		return -ENOMEM;
	}

	if (!strcmp(bt_dev.name, name)) {
		return 0;
	}

	strncpy(bt_dev.name, name, sizeof(bt_dev.name) - 1);

	/* Update advertising name if in use */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING_NAME)) {
		struct bt_data data[] = { BT_DATA(BT_DATA_NAME_COMPLETE, name,
						strlen(name)) };
		struct bt_ad sd = { data, ARRAY_SIZE(data) };

		set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, &sd, 1);

		/* Make sure the new name is set */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
			set_advertise_enable(false);
			set_advertise_enable(true);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char buf[BT_SETTINGS_SIZE(CONFIG_BT_DEVICE_NAME_MAX - 1)];
		char *str;

		str = settings_str_from_bytes(bt_dev.name, len, buf,
						  sizeof(buf));
		if (str) {
			err = settings_save_one("bt/name", str);
			if (err) {
				BT_WARN("Unable to store name, %d", err);
			}
		}
	}

	return 0;
#else
	return -ENOMEM;
#endif
}

const char *bt_get_name(void)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	return bt_dev.name;
#else
	return CONFIG_BT_DEVICE_NAME;
#endif
}

int bt_set_id_addr(const bt_addr_le_t *addr)
{
	bt_addr_le_t non_const_addr;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		BT_ERR("Setting identity not allowed after bt_enable()");
		return -EBUSY;
	}

	bt_addr_le_copy(&non_const_addr, addr);

	return bt_id_create(&non_const_addr, NULL);
}

void bt_id_get(bt_addr_le_t *addrs, size_t *count)
{
	size_t to_copy = min(*count, bt_dev.id_count);

	memcpy(addrs, bt_dev.id_addr, to_copy * sizeof(bt_addr_le_t));
	*count = to_copy;
}

static int id_find(const bt_addr_le_t *addr)
{
	u8_t id;

	for (id = 0; id < bt_dev.id_count; id++) {
		if (!bt_addr_le_cmp(addr, &bt_dev.id_addr[id])) {
			return id;
		}
	}

	return -ENOENT;
}

static void id_create(u8_t id, bt_addr_le_t *addr, u8_t *irk)
{
	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&bt_dev.id_addr[id], addr);
	} else {
		bt_addr_le_t new_addr;

		do {
			bt_addr_le_create_static(&new_addr);
			/* Make sure we didn't generate a duplicate */
		} while (id_find(&new_addr) >= 0);

		bt_addr_le_copy(&bt_dev.id_addr[id], &new_addr);

		if (addr) {
			bt_addr_le_copy(addr, &bt_dev.id_addr[id]);
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	if (atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		u8_t zero_irk[16] = { 0 };

		if (irk && memcmp(irk, zero_irk, 16)) {
			memcpy(&bt_dev.irk[id], irk, 16);
		} else {
			bt_rand(&bt_dev.irk[id], 16);
			if (irk) {
				memcpy(irk, &bt_dev.irk[id], 16);
			}
		}
	}
#endif
	/* Only store if stack was already initialized. Before initialization
	 * we don't know the flash content, so it's potentially harmful to
	 * try to write anything there.
	 */
	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
		atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_settings_save_id();
	}
}

int bt_id_create(bt_addr_le_t *addr, u8_t *irk)
{
	int new_id;

	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		if (addr->type != BT_ADDR_LE_RANDOM ||
			!BT_ADDR_IS_STATIC(&addr->a)) {
			BT_ERR("Only static random identity address supported");
			return -EINVAL;
		}

		if (id_find(addr) >= 0) {
			return -EALREADY;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (bt_dev.id_count == ARRAY_SIZE(bt_dev.id_addr)) {
		return -ENOMEM;
	}

	new_id = bt_dev.id_count++;
	if (new_id == BT_ID_DEFAULT &&
		!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR);
	}

	id_create(new_id, addr, irk);

	return new_id;
}

int bt_id_reset(u8_t id, bt_addr_le_t *addr, u8_t *irk)
{
	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		if (addr->type != BT_ADDR_LE_RANDOM ||
			!BT_ADDR_IS_STATIC(&addr->a)) {
			BT_ERR("Only static random identity address supported");
			return -EINVAL;
		}

		if (id_find(addr) >= 0) {
			return -EALREADY;
		}
	}

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}

	if (id == bt_dev.adv_id && atomic_test_bit(bt_dev.flags,
						   BT_DEV_ADVERTISING)) {
		return -EBUSY;
	}

	if (bt_addr_le_cmp(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		int err;

		err = bt_unpair(id, NULL);
		if (err) {
			return err;
		}
	}

	id_create(id, addr, irk);

	return id;
}

int bt_id_delete(u8_t id)
{
	int err;

	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}
	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}
	if (!bt_addr_le_cmp(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		return -EALREADY;
	}

	if (id == bt_dev.adv_id && atomic_test_bit(bt_dev.flags,
						   BT_DEV_ADVERTISING)) {
		return -EBUSY;
	}

	err = bt_unpair(id, NULL);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_PRIVACY)
	memset(bt_dev.irk[id], 0, 16);
#endif
	bt_addr_le_copy(&bt_dev.id_addr[id], BT_ADDR_LE_ANY);

	if (id == bt_dev.id_count - 1) {
		bt_dev.id_count--;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
		atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_settings_save_id();
	}

	return 0;
}

bool bt_addr_le_is_bonded(u8_t id, const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(id, addr);

		/* if there are any keys stored then device is bonded */
		return keys && keys->keys;
	} else {
		return false;
	}
}

static bool valid_adv_param(const struct bt_le_adv_param *param)
{
	if (param->id >= bt_dev.id_count ||
		!bt_addr_le_cmp(&bt_dev.id_addr[param->id], BT_ADDR_LE_ANY)) {
		return false;
	}

	if (!(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		/*
		 * BT Core 4.2 [Vol 2, Part E, 7.8.5]
		 * The Advertising_Interval_Min and Advertising_Interval_Max
		 * shall not be set to less than 0x00A0 (100 ms) if the
		 * Advertising_Type is set to ADV_SCAN_IND or ADV_NONCONN_IND.
		 */
		if (bt_dev.hci_version < BT_HCI_VERSION_5_0 &&
			param->interval_min < 0x00a0) {
			return false;
		}
	}

	if (param->interval_min > param->interval_max ||
		param->interval_min < 0x0020 || param->interval_max > 0x4000) {
		return false;
	}

	return true;
}

int bt_le_adv_start_instant(const struct bt_le_adv_param *param,
		      uint8_t *ad_data, size_t ad_len,
		      uint8_t *sd_data, size_t sd_len)

{
	struct bt_hci_cp_le_set_adv_param set_param;
	bt_addr_le_t *id_addr;
	int err;

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	err = hci_api_le_set_ad_data(ad_len, ad_data);
	if (err) {
		return err;
	}

	/*
	 * We need to set SCAN_RSP when enabling advertising type that allows
	 * for Scan Requests.
	 *
	 * If any data was not provided but we enable connectable undirected
	 * advertising sd needs to be cleared from values set by previous calls.
	 * Clearing sd is done by calling set_ad() with NULL data and zero len.
	 * So following condition check is unusual but correct.
	 */
	if (sd_data ||
		(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		err = hci_api_le_set_sd_data(sd_len, sd_data);
		if (err) {
			return err;
		}
	}

	memset(&set_param, 0, sizeof(set_param));

	set_param.min_interval = sys_cpu_to_le16(param->interval_min);
	set_param.max_interval = sys_cpu_to_le16(param->interval_max);
	set_param.channel_map  = param->chan_map ? param->chan_map : 0x07;

	/* Set which local identity address we're advertising with */
	bt_dev.adv_id = param->id;
	id_addr = &bt_dev.id_addr[param->id];

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			!(param->options & BT_LE_ADV_OPT_USE_IDENTITY)) {
			err = le_set_private_addr(param->id);
			if (err) {
				return err;
			}

			if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
				set_param.own_addr_type =
					BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
			} else {
				set_param.own_addr_type = BT_ADDR_LE_RANDOM;
			}
		} else {
			/*
			 * If Static Random address is used as Identity
			 * address we need to restore it before advertising
			 * is enabled. Otherwise NRPA used for active scan
			 * could be used for advertising.
			 */
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				set_random_address(&id_addr->a);
			}

			set_param.own_addr_type = id_addr->type;
		}

		set_param.type = BT_LE_ADV_IND;
	} else {
		if (param->options & BT_LE_ADV_OPT_USE_IDENTITY) {
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = set_random_address(&id_addr->a);
			}

			set_param.own_addr_type = id_addr->type;
		} else {
			err = le_set_private_addr(param->id);
			set_param.own_addr_type = BT_ADDR_LE_RANDOM;
		}

		if (err) {
			return err;
		}

		if (sd_data && sd_len) {
			set_param.type = BT_LE_ADV_SCAN_IND;
		} else {
			set_param.type = BT_LE_ADV_NONCONN_IND;
		}
	}

	if (param->options & BT_LE_ADV_OPT_DIRECT || param->options & BT_LE_ADV_OPT_DIRECT_LOW_DUTY)
	{
		bt_addr_le_copy(&set_param.direct_addr, &param->peer_addr);
		if (param->options & BT_LE_ADV_OPT_DIRECT)
		{
			set_param.type = BT_LE_ADV_DIRECT_IND;
		}
		else if (param->options & BT_LE_ADV_OPT_DIRECT_LOW_DUTY)
		{
			set_param.type = BT_LE_ADV_DIRECT_IND_LOW_DUTY;
		}
		atomic_set_bit(bt_dev.flags, BT_DEV_DIRECT_ADVERTISING);
	}

	set_param.filter_policy = param->filter_policy;

	err = hci_api_le_adv_param(set_param.min_interval,
								set_param.max_interval,
								set_param.type,
								set_param.own_addr_type,
								set_param.direct_addr.type,
								set_param.direct_addr.a.val,
								set_param.channel_map,
								set_param.filter_policy);
	if (err) {
		return err;
	}

	err = set_advertise_enable(true);
	if (err) {
		return err;
	}

	if (!(param->options & BT_LE_ADV_OPT_ONE_TIME)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);
	}

	if (param->options & BT_LE_ADV_OPT_USE_NAME) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING_NAME);
	}

	return 0;
}

int bt_le_adv_stop_instant(void)
{
    return bt_le_adv_stop();
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
			const struct bt_data *ad, size_t ad_len,
			const struct bt_data *sd, size_t sd_len)
{
	struct bt_hci_cp_le_set_adv_param set_param;
	bt_addr_le_t *id_addr;
	struct bt_ad d[2] = {};
	int err;

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	d[0].data = ad;
	d[0].len = ad_len;

	err = set_ad(BT_HCI_OP_LE_SET_ADV_DATA, d, 1);
	if (err) {
		return err;
	}

	d[0].data = sd;
	d[0].len = sd_len;

    //fix coverity warning
    const char *name = bt_get_name();;
    struct bt_data *bt_name_tmp = (&(struct bt_data)BT_DATA(BT_DATA_NAME_COMPLETE,
							  name, strlen(name)));

	if (param->options & BT_LE_ADV_OPT_USE_NAME) {
		if (sd) {
			int i;

			/* Cannot use name if name is already set */
			for (i = 0; i < sd_len; i++) {
				if (sd[i].type == BT_DATA_NAME_COMPLETE ||
					sd[i].type == BT_DATA_NAME_SHORTENED) {
					return -EINVAL;
				}
			}
		}

		d[1].data = bt_name_tmp;
		d[1].len = 1;
	}

	/*
	 * We need to set SCAN_RSP when enabling advertising type that allows
	 * for Scan Requests.
	 *
	 * If any data was not provided but we enable connectable undirected
	 * advertising sd needs to be cleared from values set by previous calls.
	 * Clearing sd is done by calling set_ad() with NULL data and zero len.
	 * So following condition check is unusual but correct.
	 */
	if (d[0].data || d[1].data ||
		(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		err = set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, d, 2);
		if (err) {
			return err;
		}
	}

	memset(&set_param, 0, sizeof(set_param));

	set_param.min_interval = sys_cpu_to_le16(param->interval_min);
	set_param.max_interval = sys_cpu_to_le16(param->interval_max);
	set_param.channel_map  = param->chan_map ? param->chan_map : 0x07;

	/* Set which local identity address we're advertising with */
	bt_dev.adv_id = param->id;
	id_addr = &bt_dev.id_addr[param->id];

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			!(param->options & BT_LE_ADV_OPT_USE_IDENTITY)) {
			err = le_set_private_addr(param->id);
			if (err) {
				return err;
			}

			if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
				set_param.own_addr_type =
					BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
			} else {
				set_param.own_addr_type = BT_ADDR_LE_RANDOM;
			}
		} else {
			/*
			 * If Static Random address is used as Identity
			 * address we need to restore it before advertising
			 * is enabled. Otherwise NRPA used for active scan
			 * could be used for advertising.
			 */
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				set_random_address(&id_addr->a);
			}

			set_param.own_addr_type = id_addr->type;
		}

		set_param.type = BT_LE_ADV_IND;
	} else {
		if (param->options & BT_LE_ADV_OPT_USE_IDENTITY) {
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = set_random_address(&id_addr->a);
			}

			set_param.own_addr_type = id_addr->type;
		} else {
			err = le_set_private_addr(param->id);
			set_param.own_addr_type = BT_ADDR_LE_RANDOM;
		}

		if (err) {
			return err;
		}

		if (sd) {
			set_param.type = BT_LE_ADV_SCAN_IND;
		} else {
			set_param.type = BT_LE_ADV_NONCONN_IND;
		}
	}

	if (param->options & BT_LE_ADV_OPT_DIRECT || param->options & BT_LE_ADV_OPT_DIRECT_LOW_DUTY)
	{
		bt_addr_le_copy(&set_param.direct_addr, &param->peer_addr);
		if (param->options & BT_LE_ADV_OPT_DIRECT)
		{
			set_param.type = BT_LE_ADV_DIRECT_IND;
		}
		else if (param->options & BT_LE_ADV_OPT_DIRECT_LOW_DUTY)
		{
			set_param.type = BT_LE_ADV_DIRECT_IND_LOW_DUTY;
		}
		atomic_set_bit(bt_dev.flags, BT_DEV_DIRECT_ADVERTISING);
	}

	set_param.filter_policy = param->filter_policy;

	err = hci_api_le_adv_param(set_param.min_interval, 
								set_param.max_interval,
								set_param.type,
								set_param.own_addr_type,
								set_param.direct_addr.type,
								set_param.direct_addr.a.val,
								set_param.channel_map,
								set_param.filter_policy);
	if (err) {
		return err;
	}

	err = set_advertise_enable(true);
	if (err) {
		return err;
	}

	if (!(param->options & BT_LE_ADV_OPT_ONE_TIME)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);
	}

	if (param->options & BT_LE_ADV_OPT_USE_NAME) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING_NAME);
	}

	return 0;
}

int bt_le_adv_stop(void)
{
	int err;

	/* Make sure advertising is not re-enabled later even if it's not
	 * currently enabled (i.e. BT_DEV_ADVERTISING is not set).
	 */
	atomic_clear_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return 0;
	}
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_DIRECT_ADVERTISING)) {
		// nothing to do
	}

	err = set_advertise_enable(false);
	if (err) {
		return err;
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* If active scan is ongoing set NRPA */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
			atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
			le_set_private_addr(bt_dev.adv_id);
		}
	}

	return 0;
}

#if defined(CONFIG_BT_OBSERVER)
static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
		param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_DISABLE &&
		param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_ENABLE) {
		return false;
	}

	if (param->interval < 0x0004 || param->interval > 0x4000) {
		return false;
	}

	if (param->window < 0x0004 || param->window > 0x4000) {
		return false;
	}

	if (param->window > param->interval) {
		return false;
	}

	return true;
}

int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	int err;

	/* Check that the parameters have valid values */
	if (!valid_le_scan_param(param)) {
		return -EINVAL;
	}

	/* Return if active scan is already enabled */
	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
			return err;
		}
	}

	if (param->filter_dup) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);
	}
	if (param->scan_filter) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCAN_FILTER);
	}

	err = start_le_scan(param->type, param->interval, param->window);
	if (err) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
		return err;
	}

	scan_dev_found_cb = cb;

	return 0;
}

int bt_le_scan_stop(void)
{
	/* Return if active scanning is already disabled */
	if (!atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	scan_dev_found_cb = NULL;

	return bt_le_scan_update(false);
}
#endif /* CONFIG_BT_OBSERVER */

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, s32_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN,
		 "Invalid buffer type requested");

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&hci_rx_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

#if defined(CONFIG_BT_BREDR)
static int br_start_inquiry(const struct bt_br_discovery_param *param)
{
	const u8_t iac[3] = { 0x33, 0x8b, 0x9e };
	struct bt_hci_op_inquiry *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_INQUIRY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	cp->length = param->length;
	cp->num_rsp = 0xff; /* we limit discovery only by time */

	memcpy(cp->lap, iac, 3);
	if (param->limited) {
		cp->lap[0] = 0x00;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_INQUIRY, buf, NULL);
}

static bool valid_br_discov_param(const struct bt_br_discovery_param *param,
				  size_t num_results)
{
	if (!num_results || num_results > 255) {
		return false;
	}

	if (!param->length || param->length > 0x30) {
		return false;
	}

	return true;
}

int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t cnt,
			  bt_br_discovery_cb_t cb)
{
	int err;

	BT_DBG("");

	if (!valid_br_discov_param(param, cnt)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return -EALREADY;
	}

	err = br_start_inquiry(param);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_INQUIRY);

	memset(results, 0, sizeof(*results) * cnt);

	discovery_cb = cb;
	discovery_results = results;
	discovery_results_size = cnt;
	discovery_results_count = 0;

	return 0;
}

int bt_br_discovery_stop(void)
{
	int err;
	int i;

	BT_DBG("");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return -EALREADY;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_INQUIRY_CANCEL, NULL, NULL);
	if (err) {
		return err;
	}

	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;
		struct bt_hci_cp_remote_name_cancel *cp;
		struct net_buf *buf;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (!priv->resolving) {
			continue;
		}

		buf = bt_hci_cmd_create(BT_HCI_OP_REMOTE_NAME_CANCEL,
					sizeof(*cp));
		if (!buf) {
			continue;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		bt_addr_copy(&cp->bdaddr, &discovery_results[i].addr);

		bt_hci_cmd_send_sync(BT_HCI_OP_REMOTE_NAME_CANCEL, buf, NULL);
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;

	return 0;
}

static int write_scan_enable(u8_t scan)
{
	struct net_buf *buf;
	int err;

	BT_DBG("type %u", scan);

	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SCAN_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, scan);
	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SCAN_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	if (scan & BT_BREDR_SCAN_INQUIRY) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ISCAN);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_ISCAN);
	}

	if (scan & BT_BREDR_SCAN_PAGE) {
		atomic_set_bit(bt_dev.flags, BT_DEV_PSCAN);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_PSCAN);
	}

	return 0;
}

int bt_br_set_connectable(bool enable)
{
	if (enable) {
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EALREADY;
		} else {
			return write_scan_enable(BT_BREDR_SCAN_PAGE);
		}
	} else {
		if (!atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EALREADY;
		} else {
			return write_scan_enable(BT_BREDR_SCAN_DISABLED);
		}
	}
}

int bt_br_set_discoverable(bool enable)
{
	if (enable) {
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ISCAN)) {
			return -EALREADY;
		}

		if (!atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EPERM;
		}

		return write_scan_enable(BT_BREDR_SCAN_INQUIRY |
					 BT_BREDR_SCAN_PAGE);
	} else {
		if (!atomic_test_bit(bt_dev.flags, BT_DEV_ISCAN)) {
			return -EALREADY;
		}

		return write_scan_enable(BT_BREDR_SCAN_PAGE);
	}
}
#endif /* CONFIG_BT_BREDR */

int bt_pub_key_gen(struct bt_pub_key_cb *new_cb)
{
	struct bt_pub_key_cb *cb;
	int err;

	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * ECC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if (!(bt_dev.supported_commands[34] & 0x02) ||
		!(bt_dev.supported_commands[34] & 0x04)) {
		BT_WARN("ECC HCI commands not available");
		return -ENOTSUP;
	}

	new_cb->_next = pub_key_cb;
	pub_key_cb = new_cb;

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return 0;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	err = hci_api_le_gen_p256_pubkey();
	if (err) {
		BT_ERR("Sending LE P256 Public Key command failed err %d", err);
		atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
		pub_key_cb = NULL;
		return err;
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
		if (cb != new_cb) {
			cb->func(NULL);
		}
	}

	return 0;
}

int bt_pub_key_regen()
{
	struct bt_pub_key_cb *cb;
	int err;

	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * ECC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if (!(bt_dev.supported_commands[34] & 0x02) ||
		!(bt_dev.supported_commands[34] & 0x04)) {
		BT_WARN("ECC HCI commands not available");
		return -ENOTSUP;
	}

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return 0;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	err = hci_api_le_gen_p256_pubkey();
	if (err) {
		BT_ERR("Sending LE P256 Public Key command failed err %d", err);
		atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
		pub_key_cb = NULL;
		return err;
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
			cb->func(NULL);
	}

	return 0;
}


const u8_t *bt_pub_key_get(void)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return pub_key;
	}

	return NULL;
}

int bt_dh_key_gen(const u8_t remote_pk[64], bt_dh_key_cb_t cb)
{
	int err = 0;

	if (dh_key_cb || atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return -EBUSY;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return -EADDRNOTAVAIL;
	}

	dh_key_cb = cb;

	err =  hci_api_le_gen_dhkey((uint8_t *)remote_pk);
	if (err)
	{
		dh_key_cb = NULL;
	}

	return err;
}

#if defined(CONFIG_BT_BREDR)
int bt_br_oob_get_local(struct bt_br_oob *oob)
{
	bt_addr_copy(&oob->addr, &bt_dev.id_addr[0].a);

	return 0;
}
#endif /* CONFIG_BT_BREDR */

int bt_le_oob_get_local(u8_t id, struct bt_le_oob *oob)
{
	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		int err;

		/* Invalidate RPA so a new one is generated */
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

		err = le_set_private_addr(id);
		if (err) {
			return err;
		}

		bt_addr_le_copy(&oob->addr, &bt_dev.random_addr);
	} else {
		bt_addr_le_copy(&oob->addr, &bt_dev.id_addr[id]);
	}

	return 0;
}

