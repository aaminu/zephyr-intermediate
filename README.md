# Zephyr Training Environment

Welcome to the Zephyr RTOS training! This repository includes a ready-to-use
development environment based on Zephyr **4.4.0**, which you can set up in one of
three ways:

---

## Manual Zephyr Setup

Follow the following guide:
- [Getting Started Guide](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html#).

Make sure to select appropriate OS and to perform all steps till
[Build the Blinky Sample](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html#build-the-blinky-sample).


*** Booting Zephyr OS build v4.4.0 ***
Counter race condition demo
[00:00:00.000,000] <inf> demo: [thread_b] Started: 0
[00:00:00.000,000] <inf> demo: [thread_b] finished: 1000000
[00:00:00.000,000] <inf> demo: [thread_c] Started: 1000000
[00:00:00.000,000] <inf> demo: [thread_c] finished: 2000000
Mutex: ENABLED
Expected total: 2000000
Actual total:   2000000
Lost updates:   0

