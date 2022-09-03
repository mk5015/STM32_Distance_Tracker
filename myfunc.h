#include <mbed.h>
#include <vector>
#include <ScopedRomWriteLock.h>

#define PI 3.1415
#define IN_TO_HEEL 30 //radius in inches from hip (origin) to heel
#define front_horizontal 1
#define front_vertical 2
#define side_left 3
#define side_right 4
#define FLASH_STORAGE 0x08080000
#define page_size 0x800


int placement;
//Blue button should be up


InterruptIn button(USER_BUTTON);
SPI spi(PF_9, PF_8, PF_7); // mosi, miso, sclk
DigitalOut cs(PC_1);       //chip select

DigitalOut led1(LED1);
DigitalOut led2(LED2);

//Timer t;
//DigitalOut LCDscreen(PD_13);

bool flag;
//Ticker t; //if (readflag) used in 0.5s intervals
//bool readflag = 0; //used with Ticker
vector<short> write_data_vec = {0};
//vector<short> write_data_y = {0};
//vector<short> write_data_z = {0};

void startstopgyro();
void flash_erase();
vector<short> flash_read(uint32_t address);
void flash_write(volatile uint32_t address, vector<short>& data);
void print_results(int distance, int readcounter, int stepcount);
void read_flash(uint8_t *data);
void save_to_flash(uint8_t *data);
//void read_flag(); //used with ticker

//used with ticker
// void read_flag(){
     //readflag = 1;
//}

//My functions
void startstopgyro()
{
    led1 = !led1;
    led2 = !led2;
    flag = !flag;
}

short read_gyro(int address)
{

    cs = 0;
    wait_us(3);
    spi.write(address);
    short low = spi.write(0x00);
    short high = spi.write(0x00);
    short reading = high << 8 | low;
    wait_us(3);
    cs = 1;

    return reading;
}

void print_results(int distance, int readcounter, int stepcount)
{   //distance = measured distance * 2 assuming equivalent strides on opposite leg as well
    printf("DISTANCE: %d.%d feet\n", (2*distance) / 12, (2*distance) % 12);

    if (stepcount > 2)
    {
        if (placement == front_horizontal || placement == front_vertical ||placement == side_left)
        {
            if (write_data_vec[readcounter - 1] < 0)
            {
                printf("Steps: %d\n", stepcount * 2 + 1);
            }
            else
            {
                printf("Steps: %d\n", stepcount * 2);
            }
        }
        else //if (placement == side_right)
        {
            if (write_data_vec[readcounter - 1] > 0)
            {
                printf("Steps: %d\n", stepcount * 2 + 1);
            }
            else
            {
                printf("Steps: %d\n", stepcount * 2);
            }
        }
    }
    else
    {
        printf("Steps: %d\n", stepcount);
    }

    int avgspd = distance / stepcount; //average distance per step
    int avgspdrem = distance % stepcount;
    printf("Average speed: %d.%d inches per step\n", avgspd, avgspdrem);
}

//FLASH FUNCTIONS HERE:
vector<short> flash_read()
{
    uint32_t* size = (uint32_t*)(0x08080000 + 40);
    vector<short> read_data_vec;
    for (uint32_t* i = (uint32_t*)0x08080000; i < size; i++)
    {
        read_data_vec.push_back(*i);
        wait_us(100);
    }

    return read_data_vec;
}

void flash_erase()
{
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(0x08080000, FLASH_VOLTAGE_RANGE_3);
    HAL_FLASH_Lock();
}

void flash_write(volatile uint32_t address,vector <short>& data)
{
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(0x08080000, FLASH_VOLTAGE_RANGE_3);
    for (int i = 0; i < data.size(); i++){
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (address+i), data[i]);
        wait_us(10000);
    }
    HAL_FLASH_Lock();
}
