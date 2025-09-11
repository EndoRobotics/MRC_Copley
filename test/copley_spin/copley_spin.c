#include "ethercat.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define IFNAME          "eth0"
#define EC_TIMEOUTMON   1000   // 1000 ms is more stable
// #define EC_TIMEOUTRXM 500  // Not defined, using EC_TIMEOUTMON for all timeouts

char IOmap[4096];
// Common velocity for all slaves
int32_t velocity = 40960; // Use the value that worked for your motor

// Object indexes for CiA 402
#define CONTROLWORD        0x6040
#define STATUSWORD         0x6041
#define MODES_OF_OPERATION 0x6060
#define TARGET_VELOCITY    0x60FF

// Helper to read SDO (16-bit)
int read_statusword(uint16_t *status, int slave)
{
  int size = sizeof(uint16_t);
  int ret  = ec_SDOread(slave, STATUSWORD, 0x00, FALSE, &size, status, EC_TIMEOUTMON); // Use EC_TIMEOUTMON
  if (ret <= 0)
  {
    printf("Failed to read statusword from slave %d. Error: %d\n", slave, ret);
    return -1;
  }
  return 0;
}

// Helper to read a 16-bit word from a specific SDO index
// (Note: For 0x6080, which is typically INTEGER32, you'd need a read_int32 function)
int read_word(uint16_t *value, int slave, uint16_t index, uint8_t subindex)
{
  int size = sizeof(uint16_t);
  int ret  = ec_SDOread(slave, index, subindex, FALSE, &size, value, EC_TIMEOUTMON); // Use EC_TIMEOUTMON
  if (ret <= 0)
  {
    printf("Failed to read word from index 0x%04X:%02X on slave %d. Error: %d\n", index, subindex, slave, ret);
    return -1;
  }
  return 0;
}

// Helper to write 16-bit SDO
int write_controlword(uint16_t cw, int slave)
{
  int ret = ec_SDOwrite(slave, CONTROLWORD, 0x00, FALSE, sizeof(cw), &cw, EC_TIMEOUTMON); // Use EC_TIMEOUTMON
  if (ret <= 0)
  {
    printf("Failed to write controlword 0x%04X to slave %d. Error: %d\n", cw, slave, ret);
    return -1;
  }
  return 0;
}

// Helper to write 8-bit SDO
int write_mode(uint8_t mode, int slave)
{
  int ret = ec_SDOwrite(slave, MODES_OF_OPERATION, 0x00, FALSE, sizeof(mode), &mode, EC_TIMEOUTMON); // Use EC_TIMEOUTMON
  if (ret <= 0)
  {
    printf("Failed to write mode of operation %d to slave %d. Error: %d\n", mode, slave, ret);
    return -1;
  }
  return 0;
}

// Helper to write 32-bit target velocity SDO
int write_velocity(int32_t velocity, int slave)
{
  int ret = ec_SDOwrite(slave, TARGET_VELOCITY, 0x00, FALSE, sizeof(velocity), &velocity, EC_TIMEOUTMON); // Use EC_TIMEOUTMON
  if (ret <= 0)
  {
    printf("Failed to write target velocity %d to slave %d. Error: %d\n", velocity, slave, ret);
    return -1;
  }
  return 0;
}

// Wait for a specific statusword mask & expected value with timeout
int wait_statusword_mask(uint16_t mask, uint16_t expected, int slave, int timeout_ms)
{
  uint16_t status  = 0;
  int      elapsed = 0;
  printf("Waiting for statusword on slave %d: (0x%04X & 0x%04X) == 0x%04X\n", slave, mask, expected, expected);
  while (elapsed < timeout_ms)
  {
    if (read_statusword(&status, slave) != 0)
      return -1;
    printf("Current Statusword: 0x%04X\n", status);
    if ((status & mask) == expected)
    {
      printf("Statusword condition met.\n");
      return 0;
    }
    usleep(100000); // 100 ms
    elapsed += 100;
  }
  printf("Timeout waiting for statusword (0x%04X & 0x%04X) == 0x%04X. Last status: 0x%04X\n", mask, expected, expected, status);
  return -1;
}

int main()
{
  printf("Starting SDO-only motor spin test on %s for multiple slaves\n", IFNAME);

  if (!ec_init(IFNAME))
  {
    printf("Failed to initialize %s\n", IFNAME);
    return 1;
  }

  // Configure slaves, TRUE for verbose output
  if (ec_config_init(TRUE) <= 0)
  {
    printf("No slaves found or PDO config failed!\n");
    ec_close();
    return 1;
  }

  // Configure mapping for PDOs
  ec_config_map(&IOmap);
  // Configure distributed clocks (DC)
  ec_configdc();

  // Define the slave IDs to control. Assuming slave 1 and slave 2.
  // Adjust this array if your slaves have different indices or you want to control more/fewer.
  int slaves_to_control[]   = { 1, 2 , 3, 4};
  int num_slaves_to_control = sizeof(slaves_to_control) / sizeof(slaves_to_control[0]);

  // Check if sufficient slaves are found
  if (ec_slavecount < num_slaves_to_control)
  {
    printf("Not enough slaves found. Found %d, need %d.\n", ec_slavecount, num_slaves_to_control);
    goto cleanup;
  }

  // Request SAFE_OP state for all slaves (ec_slave[0] represents all slaves)
  ec_slave[0].state = EC_STATE_SAFE_OP;
  ec_writestate(0);
  if (ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTMON) != EC_STATE_SAFE_OP)
  {
    printf("Failed to enter SAFE_OP state for all slaves.\n");
    ec_close();
    return 1;
  }
  printf("All slaves in SAFE_OP state.\n");

  sleep(1); // Give some time for state transition

  // Request OPERATIONAL state for all slaves
  ec_slave[0].state = EC_STATE_OPERATIONAL;
  ec_writestate(0);

  for (int i = 0; i < 40; i++) 
  { // Loop for 40 cycles, adjust as needed
    ec_send_processdata();

    if (ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTMON) == EC_STATE_OPERATIONAL)
    {
      printf("All slaves in OPERATIONAL state.\n");
      break;
    }
    else
    {
      printf("Failed to enter OPERATIONAL state for all slaves.\n");
      for(i = 1; i <= ec_slavecount; i++) 
      {
        printf("Slave %d state: 0x%02X, status code: 0x%04X\n",
              i, ec_slave[i].state, ec_slave[i].ALstatuscode);
      }
      osal_usleep(1000);
    }

    if (i == 40)
    {
      ec_close();
      return 1;
    }
  }

  // --- Configuration and Enable Operation for Each Slave ---
  for (int i = 0; i < num_slaves_to_control; i++)
  {
    osal_usleep(1000);
    int current_slave = slaves_to_control[i];
    printf("\n--- Configuring Slave %d ---\n", current_slave);

    osal_usleep(100);
    // Step 1: Shutdown (Controlword = 0x06) - Transition to Ready to Switch On
    printf("Sending Shutdown (0x06) to slave %d...\n", current_slave);
    if (write_controlword(0x06, current_slave) != 0)
      goto cleanup;
    if (wait_statusword_mask(0x006F, 0x0021, current_slave, 2000) != 0) // Ready to Switch On
      goto cleanup;

    osal_usleep(100);
    // Step 2: Set mode of operation = Profile Velocity (3)
    printf("Setting Mode of Operation to Profile Velocity (3) for slave %d...\n", current_slave);
    if (write_mode(3, current_slave) != 0)
      goto cleanup;

    osal_usleep(100);
    // Step 3: Switch On (Controlword = 0x07) - Transition to Switched On
    printf("Sending Switch On (0x07) to slave %d...\n", current_slave);
    if (write_controlword(0x07, current_slave) != 0)
      goto cleanup;
    if (wait_statusword_mask(0x006F, 0x0023, current_slave, 2000) != 0) // Switched On
      goto cleanup;

    osal_usleep(100);
    // Step 4: Enable Operation (Controlword = 0x0F) - Transition to Operation Enabled
    printf("Sending Enable Operation (0x0F) to slave %d...\n", current_slave);
    if (write_controlword(0x0F, current_slave) != 0)
      goto cleanup;
    if (wait_statusword_mask(0x006F, 0x0027, current_slave, 2000) != 0) // Operation Enabled
      goto cleanup;

    printf("Slave %d is now in Operation Enabled state.\n", current_slave);
  }

  printf("\nAll configured slaves are in Operation Enabled state. Starting synchronized motion.\n");

  // --- Apply Velocity Command to All Slaves ---
  // This is where you trigger motion for all slaves simultaneously.
  for (int i = 0; i < num_slaves_to_control; i++)
  {
    osal_usleep(100);
    int current_slave = slaves_to_control[i];
    velocity *= (current_slave == 1) ? 1 : -1;
    printf("Setting target velocity to %d for slave %d\n", velocity, current_slave);
    if (write_velocity(velocity, current_slave) != 0)
      goto cleanup;
  }

  // Let motors spin for 30 seconds
  printf("Motors spinning for 5 seconds...\n");
  sleep(5);

  // --- Stop Motors (Velocity = 0) ---
  printf("Stopping motors...\n");
  for (int i = 0; i < num_slaves_to_control; i++)
  {
    osal_usleep(1000);
    int current_slave = slaves_to_control[i];
    printf("Stopping slave %d\n", current_slave);
    if (write_velocity(0, current_slave) != 0)
      goto cleanup;
  }

  // --- Disable Drives (Shutdown sequence) ---
  printf("Disabling drives (Controlword 0x07 then 0x06)...\n");
  for (int i = 0; i < num_slaves_to_control; i++)
  {
    osal_usleep(100);
    // CORRECTED TYPO HERE: was slaves_slaves_to_control[i]
    int current_slave = slaves_to_control[i];
    printf("Disabling slave %d...\n", current_slave);
    // Transition from Operation Enabled to Switched On (Controlword = 0x07)
    if (write_controlword(0x07, current_slave) != 0)
      goto cleanup;
    if (wait_statusword_mask(0x006F, 0x0023, current_slave, 2000) != 0) // Wait for Switched On
      goto cleanup;

    // Transition from Switched On to Ready to Switch On (Controlword = 0x06)
    if (write_controlword(0x06, current_slave) != 0)
      goto cleanup;
    if (wait_statusword_mask(0x006F, 0x0021, current_slave, 2000) != 0) // Wait for Ready to Switch On
      goto cleanup;
  }

  printf("Test finished successfully for all slaves.\n");

cleanup:
  printf("Closing EtherCAT connection.\n");
  ec_close();
  return 0;
}
