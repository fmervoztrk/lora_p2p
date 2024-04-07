#include <Time.h> 
#include <TimeLib.h>

long startTime;
bool rx_done = false;
double myFreq = 868000000;
uint16_t sf = 12, bw = 0, cr = 0, preamble = 8, txPower = 22;

time_t t;
struct tm *tm;

void hexDump(uint8_t * buf, uint16_t len)//gönderilen mesajı hexadecimal hale çevirir( HEX to ASCII)
{
    char alphabet[17] = "0123456789abcdef";
    Serial.print(F("   +------------------------------------------------+ +----------------+\r\n"));
    Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\r\n"));
    for (uint16_t i = 0; i < len; i += 16) {
        if (i % 128 == 0)
            Serial.print(F("   +------------------------------------------------+ +----------------+\r\n"));
        char s[] = "|                                                | |                |\r\n";
        uint8_t ix = 1, iy = 52;
        for (uint8_t j = 0; j < 16; j++) {
            if (i + j < len) {
  	            uint8_t c = buf[i + j];
  	            s[ix++] = alphabet[(c >> 4) & 0x0F];
  	            s[ix++] = alphabet[c & 0x0F];
  	            ix++;
  	            if (c > 31 && c < 128)
  	                s[iy++] = c;
  	            else
  	                s[iy++] = '.';
            }
        }
        uint8_t index = i / 16;
        if (i < 256)
            Serial.write(' ');
        Serial.print(index, HEX);
        Serial.write('.');
        Serial.print(s);
    }
    Serial.print(F("   +------------------------------------------------+ +----------------+\r\n"));
}

/*
  typedef struct rui_lora_p2p_revc {
  // Pointer to the received data stream
  uint8_t *Buffer;
  // Size of the received data stream
  uint8_t BufferSize;
  // Rssi of the received packet
  int16_t Rssi;
  // Snr of the received packet
  int8_t Snr;
  } rui_lora_p2p_recv_t;
*/
void recv_cb(rui_lora_p2p_recv_t data)
{
    rx_done = true;//transmitter tarafından gönderilen mesaj alındı.
    if (data.BufferSize == 0) {//mesaj içeriğinin boş olup olmadığı kontrol edildi
        Serial.println("Empty buffer.");
        return;
    }
    char buff[92];
    sprintf(buff, "Incoming message, length: %d, RSSI: %d, SNR: %d",
  	    data.BufferSize, data.Rssi, data.Snr);//alınan mesajın özellikleri bulunur
    Serial.println(buff);
    hexDump(data.Buffer, data.BufferSize);//alınan mesajı Hexadecimal hale çevirir.
}

void send_cb(void)
{//modülü alıcı moduna ayarlar
    Serial.printf("P2P set Rx mode %s\r\n",
		api.lorawan.precv(65534) ? "Success" : "Fail");//This command is used to set the P2P RX mode and timeout for RX window.
}

void setup()
{
    Serial.begin(115200);
    Serial.println("RAKwireless LoRaWan P2P Example");
    Serial.println("------------------------------------------------------");
    delay(2000);
    startTime = millis();
    
    if(api.lorawan.nwm.get() != 0)
    {
        Serial.printf("Set Node device work mode %s\r\n",//This command is used to switch to LoRaWAN® or (P2P)point-to-point mode.
            api.lorawan.nwm.set(0) ? "Success" : "Fail");//0 P2P or 1 LoRaWAN
        api.system.reboot();//RAKwireless RUI3 modülünü yeniden başlatmak için kullanılır.
    }

    Serial.println("P2P Start");
    Serial.printf("Hardware ID: %s\r\n", api.system.chipId.get().c_str());
    Serial.printf("Model ID: %s\r\n", api.system.modelId.get().c_str());
    Serial.printf("RUI API Version: %s\r\n",
  		api.system.apiVersion.get().c_str());
    Serial.printf("Firmware Version: %s\r\n",
  		api.system.firmwareVersion.get().c_str());
    Serial.printf("AT Command Version: %s\r\n",
  		api.system.cliVersion.get().c_str());
    Serial.printf("Set P2P mode frequency %3.3f: %s\r\n", (myFreq / 1e6),
  		api.lorawan.pfreq.set(myFreq) ? "Success" : "Fail");//This command is used to access and configure P2P mode frequency.
    Serial.printf("Set P2P mode spreading factor %d: %s\r\n", sf,
  		api.lorawan.psf.set(sf) ? "Success" : "Fail");//This command is used to access and configure P2P mode spreading factor.
    Serial.printf("Set P2P mode bandwidth %d: %s\r\n", bw,
  		api.lorawan.pbw.set(bw) ? "Success" : "Fail");//This command is used to access and configure P2P mode bandwidth.
    Serial.printf("Set P2P mode code rate 4/%d: %s\r\n", (cr + 5),
  		api.lorawan.pcr.set(cr) ? "Success" : "Fail");//This command is used to access and configure P2P mode coding rate.
    Serial.printf("Set P2P mode preamble length %d: %s\r\n", preamble,
  		api.lorawan.ppl.set(preamble) ? "Success" : "Fail");//This command is used to access and configure P2P mode preamble length.
    Serial.printf("Set P2P mode tx power %d: %s\r\n", txPower,
  		api.lorawan.ptp.set(txPower) ? "Success" : "Fail");//This command is used to access and configure P2P mode TX power (0 = lowest, 22 = highest).
    api.lorawan.registerPRecvCallback(recv_cb);
    api.lorawan.registerPSendCallback(send_cb);
    Serial.printf("P2P set Rx mode %s\r\n",
  		api.lorawan.precv(65534) ? "Success" : "Fail");
    //value is set to 65534, the device will continuously listen to P2P LoRa packets without any timeout.
}

void loop()
{
    uint8_t message[] = "Hello";
    bool send_result = false;

    if (rx_done) {
        rx_done = false;
        while (!send_result) {
            send_result = api.lorawan.psend(sizeof(message), message);
            Serial.printf("P2P send %s\r\n", send_result ? "Success" : "Fail");
            if (!send_result) {
                Serial.printf("P2P finish Rx mode %s\r\n", api.lorawan.precv(0) ? "Success" : "Fail");
                delay(1000);
//00.05 - 11.44 
                t = now();
                tm = localtime(&t);
                char timeStr[20];
                strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm);
                Serial.println(timeStr);
            }
        }
    }
    
    delay(500);
}
