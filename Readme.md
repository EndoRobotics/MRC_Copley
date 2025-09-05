# MRC Copley Motor Driver test 1

## 06 June 2025

### Installation

1. Please git clone the entire folder into the system

### Compile

1. cd into the folder

   ```bash
   :~/. $ cd /MRC_Copley_test
   :~/MRC_Copley_test $
   ```
2. compile the file by make

   ```bash
   :~/. $ apt-get update 
   :~/. $ apt-get install -y git make libpcap-dev build-essential 
   :~/MRC_Copley_test $ make
   ```
3. You should see this message if its sucessed ``[SUCCESS] All targets built successfully!``

### Execute

You will see two files under the ``bin`` folder

```bash
bin
├── copley_spin
│   └── copley_spin
└── slave_detect
    └── slave_detect
```

#### Slave Detect

this programme will help to check all the slave exisiting inside the ``eth0`` interface.

```bash
:~/MRC_Copley_test $ sudo ./bin/slave_detect/slave_detect
```

and the output should be similar to the following depends on how many slaves in the system

```bash
:~/MRC_Copley_test $
Starting on interface eth0
    __  _______  ______   ______            __                              __                    __     _
   /  |/  / __ \/ ____/  / ____/___  ____  / /__  __  __   ____ ___  ____  / /_____  _____   ____/ /____(_)   _____  _____
  / /|_/ / /_/ / /      / /   / __ \/ __ \/ / _ \/ / / /  / __ `__ \/ __ \/ __/ __ \/ ___/  / __  / ___/ / | / / _ \/ ___/
 / /  / / _, _/ /___   / /___/ /_/ / /_/ / /  __/ /_/ /  / / / / / / /_/ / /_/ /_/ / /     / /_/ / /  / /| |/ /  __/ /
/_/  /_/_/ |_|\____/   \____/\____/ .___/_/\___/\__, /  /_/ /_/ /_/\____/\__/\____/_/      \__,_/_/  /_/ |___/\___/_/
                                 /_/           /____/

EtherCAT interface eth0 initialized.
Number of slaves found: 2

Idx   Slave Name                     Address (Phys)  Latency (ns)
---------------------------------------------------------------------
1     AEV                            0x1001         0
2     AEV                            0x1002         0
```

#### Copley Spin

> ***! Please ensure the motor is free to spin before running this programme !***

this programme will spin all motor with different direction for 30 seconds

```bash
:~/MRC_Copley_test$ sudo ./bin/copley_spin/copley_spin
```

and you should see the terminal shows the message similar to 

```bash
:~/MRC_Copley_test$ 
Starting SDO-only motor spin test on eth0 for multiple slaves
Found 2/10.
All slaves in SAFE_OP state.
All slaves in OPERATIONAL state.

--- Configuring Slave 1 ---
...
...
...
```

and all motor should be spining
