import numpy as np
import os,sys

filename = "pwm_freqs.npy"

def main(argv):
    if os.path.exists(filename):
        pwm_freqs = np.load(filename)
    else:
        freq_source = 16000000
        pwm_freqs = np.ones((8, 65535),dtype=float)
        pwm_freqs[0] = freq_source / 1 / np.arange(1, 65536, 1)
        pwm_freqs[1] = freq_source / 2 / np.arange(1, 65536, 1)
        pwm_freqs[2] = freq_source / 4 / np.arange(1, 65536, 1)
        pwm_freqs[3] = freq_source / 8 / np.arange(1, 65536, 1)
        pwm_freqs[4] = freq_source / 16 / np.arange(1, 65536, 1)
        pwm_freqs[5] = freq_source / 32 / np.arange(1, 65536, 1)
        pwm_freqs[6] = freq_source / 64 / np.arange(1, 65536, 1)
        pwm_freqs[7] = freq_source / 128 / np.arange(1, 65536, 1)
        np.save(filename, pwm_freqs)

    try:
        freq = float(argv[1])
    except:
        print('invalid frequency input!')
        return
    pos = np.unravel_index(np.argmin(np.abs(pwm_freqs - freq)), pwm_freqs.shape)
    print("best configuraton is \033[31mPWM.CLK_DIV={0}\033[0m, \033[31mPWM.reload={1}\033[0m".format(pos[0]+1, pos[1]+1))
    error = abs(freq - pwm_freqs[pos[0]][pos[1]])/freq
    print("\033[36mdesired frequency = {0:.2F}, actual frequency = {1:.2F}, error = {2:.2%}\033[0m".format(freq, pwm_freqs[pos[0]][pos[1]], error))


if __name__ == "__main__":
    if (len(sys.argv) == 1):
        print("PHY6212 PWM Frequency Calculator")
        print("usage: get_proximate_pwm_frequency freq")
    else:
        main(sys.argv)
