#include <mbed.h>
#include <vector>
#include <ScopedRomWriteLock.h>
#include "myfunc.h"

//flash_init();
//save_to_flash((int*) write_data);
//int flash_init();
// Documents
// Manual for dev board: https://www.st.com/resource/en/user_manual/um1670-discovery-kit-with-stm32f429zi-mcu-stmicroelectronics.pdf
// gyroscope datasheet: https://www.mouser.com/datasheet/2/389/dm00168691-1798633.pdf

int main()
{
    led1 = 0; //Green when Tracking
    led2 = 1; //Red when not
    volatile int stepcount = 0;
    volatile int distance = 0;
    volatile int readcounter = 0;
    volatile double linearvelocity = 0;
    // float read_number = 0.0f;
    // float *pointer_read = &read_number;
    // read_flash((uint8_t*)pointer_read);

    //Read out data stored in flash
    vector<short> read_data_vec = {0};
    read_data_vec = flash_read();
    wait_us(200000);

    //previously stored data
    for (int i = 0; i < 40; i++)
    {
        // storedinflash = flash_read(FLASH_SECTOR_11 + i);
        wait_us(10000);
        printf("FLASH: %hd at %d\n", read_data_vec[i], i);
    }

    flash_erase();

    // Setup the spi for 8 bit data, high steady state clock,
    cs = 1;
    spi.format(8, 3);
    spi.frequency(1000000);

    //set sensitivity to 500 (0x10)
    cs = 0;
    spi.write(0x23);
    spi.write(0x10); //0x00 would be 245 dps
    cs = 1;

    //check if it is reading from the correct source before initating any isr
    cs = 0;
    spi.write(0x8F);
    int whoami = spi.write(0x00);
    cs = 1;
    if (whoami == 0xD3)
        printf("Gyroscope connected\n\n");
    else
        printf("Not connected to gyroscope\n\n");

    //Set placement to chosen configuration (front vertical, front horizontal, side left, side right)
    //to endure the correct axis is measured and used in calculations
    placement = front_vertical;

    //mbed_mpu_manager_unlock_ram_execution();
    //mbed_mpu_manager_unlock_rom_write();
    button.fall(&startstopgyro);
    //t.attach(&read_flag, 0.5); //if using ticker uncomment readflag code in while loop
    while (1)
    {
        //Allow ROM write on the STM32F4 board
        ScopedRomWriteLock make_ram_executable;
        cs = 0;
        wait_us(3);
        spi.write(0x20);
        spi.write(0x0F);
        wait_us(3);
        cs = 1;
        while (flag == 1 && readcounter < 40)
        {
            //ScopedRomWriteLock make_ram_executable;
            //initialize control register to 1111, all active/read accessible


            //if (readflag == 1){
                //Remember that the gyro is reading the angular velocity around a midpoint in the sensor
                //X is side to side when blue button is up and black is down horizontal placement
                short xReading = read_gyro(0xE8);
                short yReading = read_gyro(0xEA);
                short zReading = read_gyro(0xEC);
                wait_us(500000); //manual 0.5 second increments

                //depending on which placement, the other two axes are essentially noise

                printf("%hd %hd %hd\n", (short)(xReading * 0.0175), (short)(yReading * 0.0175), (short)(zReading * 0.0175));

                //raw data converted based on sensitivity
                //conversion factor for 500 is 17.5 and 245 is 8.75 to get dps
                if (placement == front_vertical)
                {
                    write_data_vec.push_back(xReading);
                }
                else if (placement == front_horizontal)
                {
                    write_data_vec.push_back(yReading);
                }
                else //side left or side right
                {
                    write_data_vec.push_back(zReading);
                }

                readcounter++;

                if (write_data_vec[readcounter] > 0 && write_data_vec[readcounter - 1] < 0)
                {
                    //increment step count whenever the direction of the angular velocity changes
                    stepcount++;
                }
                //readflag = 0;
            //}
        }

        //flash_write(readcounter, write_data_x[readcounter]);
        wait_us(1000000);

        //calculate distance and print results
        if (readcounter > 0)
        {
            led1 = 0;
            led2 = 1;

            flash_write(0x08080000, write_data_vec);

            for (int i = 0; i < readcounter; i++)
            {

                if (placement == front_horizontal || placement == front_vertical || placement == side_left)
                {
                    if (write_data_vec[i] > 0)
                    { //using push off readings HORIZONTAL
                        linearvelocity = (double)(write_data_vec[i]) * 0.0175 * (PI / 180.0);
                        distance = distance + (int)(linearvelocity * (IN_TO_HEEL)*0.5);
                    }
                }

                else //(placement == side_right)
                {
                    if (write_data_vec[i] < 0)
                    { // attached to side of right thigh
                        linearvelocity = (double)(write_data_vec[i]) * 0.0175 * (PI / 180.0);
                        distance = distance - (int)(linearvelocity * (IN_TO_HEEL)*0.5);
                    }
                }
            }

            print_results(distance, readcounter, stepcount);

            //Reset flag, results, and accumulating counters after output results
            flag = 0;
            distance = 0;
            stepcount = 0;
            readcounter = 0;

            write_data_vec = {0};
        }
    }
}
