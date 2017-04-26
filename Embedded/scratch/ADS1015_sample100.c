/* 
    ADS1115_sample.c - 12/9/2013. Written by David Purdie as part of the openlabtools initiative
    Initiates and reads a single sample from the ADS1115 (without error handling)
*/

#include <stdio.h>
#include <fcntl.h>     // open
#include <linux/i2c-dev.h> // I2C bus definitions



int main() {
    
  unsigned char m_haladc_address_ui8 = 0x48;    // Address of our device on the I2C bus
  int m_I2CFile_fp;
  
  unsigned char m_writeBuf_rg24[3];        // Buffer to store the 3 bytes that we write to the I2C device
  unsigned char m_readBuf_rg16[2];        // 2 byte buffer to store the data read from the I2C device
  
  unsigned short l_val_ui16 = 0;        // Stores the 16 bit value of our ADC conversion

  unsigned short l_counter_ui16 = 0;        // Number of read values (max. 65536)

  float l_sum_f32 = 0;        // Summing the values up and stores the average at the end

  unsigned short l_number_ui16 = 10000;        // Number of values  (max. 65536)

  FILE *outputfile;        // File to store the measured values


  
  outputfile=fopen("MeasureValues16.txt","a");
  if(outputfile == NULL)
    {
    printf("File could not be opened");
    return(1);
    }
  
  m_I2CFile_fp = open("/dev/i2c-1", O_RDWR);        // Open the I2C device
  
  ioctl(m_I2CFile_fp, I2C_SLAVE, m_haladc_address_ui8);   // Specify the address of the I2C Slave to communicate with
      
  // These three bytes are written to the ADS1115 to set the config register and start a conversion 
  m_writeBuf_rg24[0] = 1;            // This sets the pointer register so that the following two bytes write to the config register
  m_writeBuf_rg24[1] = 0xC2;       // This sets the 8 MSBs of the config register (bits 15-8) to 11000011
  m_writeBuf_rg24[2] = 0x23;          // This sets the 8 LSBs of the config register (bits 7-0) to ????   <<<<<<
  
  // Initialize the buffer used to read data from the ADS1115 to 0
  m_readBuf_rg16[0]= 0;        
  m_readBuf_rg16[1]= 0;


          
  // Write m_writeBuf_rg24 to the ADS1115, the 3 specifies the number of bytes we are writing,
  // this begins a single conversion
  write(m_I2CFile_fp, m_writeBuf_rg24, 3);    

  while(l_counter_ui16 != l_number_ui16){
    
      
  read(m_I2CFile_fp, m_readBuf_rg16, 2);    // Read the config register into readBuf
     

  m_writeBuf_rg24[0] = 0;            // Set pointer register to 0 to read from the conversion register
  write(m_I2CFile_fp, m_writeBuf_rg24, 1);
 
  read(m_I2CFile_fp, m_readBuf_rg16, 2);        // Read the contents of the conversion register into readBuf
    

  l_val_ui16 = m_readBuf_rg16[0] << 8 | m_readBuf_rg16[1];    // Combine the two bytes of readBuf into a single 16 bit result 
  l_val_ui16 = l_val_ui16 >> 4;      

  //printf("Hex: %lx \n",l_val_ui16);        // Hexwert ausgeben
  //printf("Voltage Reading %f (V) \n", (float)l_val_ui16*4.096/32767.0);    // Print the result to terminal, first convert from binary value to mV
  //printf("%i Voltage Reading %f (V) \n",l_counter_ui16, (float)l_val_ui16*4.096/2047.0);    // Print the result to terminal, first convert from binary value to mV

  l_sum_f32 = l_sum_f32 + l_val_ui16;
  fprintf(outputfile,"%f,\n",(float)l_val_ui16*4.096/2047.0);
 
  while (((m_readBuf_rg16[0] << 8 | m_readBuf_rg16[1])>>4) == l_val_ui16)    // Wait for value to change
      {
        read(m_I2CFile_fp, m_readBuf_rg16, 2);
        //printf ("Schleife");
    }

 l_counter_ui16++;            // increase counted cycles
    
  }
  
  
  l_sum_f32 = l_sum_f32*4.096/2047.0;
  l_sum_f32 = l_sum_f32 / l_number_ui16;

  printf("Mittelwert aus %i: %f \n5V Wert: %f\n",l_number_ui16,l_sum_f32,(l_sum_f32/0.66));
        
  close(m_I2CFile_fp);
  
  return 0;

}


