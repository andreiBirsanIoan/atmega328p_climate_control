/* Sistem monitorizare temperatura BARE-METAL DHT11 ADC pe ATmega328P (16 MHz) 
  -Citire bare-metal de pe senzorul DHT11-pasi:
    A. Semnal de start de la Arduino:
      -setare directie OUTPUT pentru pinul DHT11
      -setare valoare pin DHT LOW minim 18ms
      -setare pin DHT HIGH 20-40 us (am ales varianta de mijloc 30us)
      -directie pin DHT input
    B. Raspuns DHT11:
      -Stare LOW 80 us
      -Stare HIGH 80us
    C. Transmitere date:
      -40 biti impartiti in 5 bytes:
        ->Umiditate (parte intreaga)
        ->Umiditate (parte zecimala)
        ->Temperatura (parte intreaga)
        ->Temperatura (parte zecimala)
        ->Checksum (Suma de control a celorlalti 4 bytes)
        ->Daca Checksum=Suma celor 4 bytes=> date valide
  -Monitorizare prag folosind potentiometru montat pe pinul A0, citire cu analogRead
  -Formatare potentiometru pentru pragul de maxim 42 grade Celsius (in mod normal valori 0-1023)
  -Verificare temp > prag => (DA): pornesc ventilatorul conectat la releul de pe PD7 (releul are logica Active-LOW, adica porneste cand valoarea setata e 0)
                          => (NU): nu fac nimic
  -formatare valori: temperatura: temp_intreg*10+temp_zecimal, potentiometru: (analogRead(A0)/24)*10
  -eventuale modificari: - Adaugare Display OLED pentru afisare prag setat si temperatura + umiditate; 
                         - Adaugare Buzzer pentru depasire prag normal temperatura
  -citire executata la fiecare 5 secunde (perioada calculata folosind un timer care vine la 1ms) 
*/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
volatile uint16_t contor=0;
void setup()
{
  ADCSRA |= (1 << ADEN);
  DDRC &= ~0x01;
  TCCR1A=0;
  TCCR1B=0b00001011;
  TIMSK1 |= 0x02;
  OCR1A=250;
  interrupts();
}

ISR(TIMER1_COMPA_vect){
  contor++;
}

uint8_t dht_date[5];
uint8_t citireDHT11_BareMetal() {
  uint8_t i, j;
  uint8_t timp_counter;

  for (i = 0; i < 5; i++) dht_date[i] = 0;

  //START 
  DDRD |= (1 << PD2) |(1 << PD7);     // PD2 Output (DHT11 ) si PD7 Output (Releu)
  
  PORTD &= ~(1 << PD2);   // Setez PD2 pe LOW
  _delay_ms(20);          // 20ms LOW (Senzorul DHT11 are nevoie sa stea minim 18ms pe LOW la inceput)

  PORTD |= (1 << PD2);    // Setare DHT11 pin HIGH
  _delay_us(30);          // 30us HIGH

  DDRD &= ~(1 << PD2);    // Schimb directia DHT11 pin pe INPUT
  PORTD |= (1 << PD2);    // Activează Pull-up intern de siguranță

  // Asteptam semnalul de LOW de la senzor(~80us)
  timp_counter = 0;
  while (PIND & (1 << PD2)) {
    _delay_us(1);
    if (++timp_counter > 100) return 1; // Cod eroare 1: Senzorul nu a tras linia în LOW (Răspuns lipsă)
  }

  // Asteptam semnalul HIGH de la senzor (~80us)
  timp_counter = 0;
  while (!(PIND & (1 << PD2))) {
    _delay_us(1);
    if (++timp_counter > 100) return 2; // Cod eroare 2: Senzorul a rămas blocat în LOW
  }

  // Așteptăm terminarea semnalului HIGH al răspunsului
  timp_counter = 0;
  while (PIND & (1 << PD2)) {
    _delay_us(1);
    if (++timp_counter > 100) return 3; // Cod eroare 3: Senzorul a rămas blocat în HIGH
  }
  //Citirea semnalului de 40 de biti de la senzor
  for (j = 0; j < 40; j++) {
    // Așteptăm să se termine starea LOW de dinaintea fiecarui bit (~50us)
    timp_counter = 0;
    while (!(PIND & (1 << PD2))) {
      _delay_us(1);
      if (++timp_counter > 100) return 4; // Cod eroare 4: Timeout la debutul bitului
    }
    // Măsurăm durata stării HIGH a bitului
    timp_counter = 0;
    while (PIND & (1 << PD2)) {
      _delay_us(1);
      if (++timp_counter > 100) return 5; // Cod eroare 5: Line rămasă HIGH în timpul citirii biților
    }
    //Verificare stare bit, un bit are valoarea '0' daca durata semnalului HIGH este de ~26-28us
    //Daca durata este ~70 us, atunci bitul are valoarea '1'
    if (timp_counter > 40) {
      dht_date[j / 8] |= (1 << (7 - (j % 8))); //adaugare biti
    }
  }
  // Checksum Test
  uint8_t sum = dht_date[0] + dht_date[1] + dht_date[2] + dht_date[3];
  if (sum != dht_date[4]) {
    return 6; //Cod eroare 6: Checksum Invalid (date alterate)
  }
  return 0;
}
uint16_t valoare=(analogRead(A0)/24)*10;

int main(void) {
  setup();
  PORTD &= ~0x80;
  PORTC &= ~0x01;
  Serial.begin(9600);
  _delay_ms(1000); // Timeout de stabilizare
  uint16_t ultimul_timp=0;
  while (1) {
    if(contor-ultimul_timp>=5000){
      ultimul_timp=contor;
    uint8_t rezultat = citireDHT11_BareMetal();
    valoare=(analogRead(A0)/24)*10; //citire valoare potentiometru
    if (rezultat == 0) {
      
      Serial.print("Umiditate: ");
      Serial.print(dht_date[0]);
      Serial.print(".");
      Serial.print(dht_date[1]);
      Serial.print("% | Temp: ");
      Serial.print(dht_date[2]);
      Serial.print(".");
      Serial.print(dht_date[3]);
      Serial.println(" C");  
      uint16_t temp=dht_date[2]*10+dht_date[3];// creez temperatura totala (parte intreaga + zecimala)
      if(temp<valoare){
        PORTD |= 0x80;  //daca este mai mic decat acel prag, nu face nimic
      }
      else PORTD &= ~0x80; // daca temperatura este mai mare, porneste ventilatorul
    } else {
      Serial.print("Eroare Cod [");
      Serial.print(rezultat);
      Serial.print("] - ");

      switch(rezultat) {
        case 1: Serial.println("Senzorul nu raspunde (Verifica conexiunile/Pull-up)."); break;
        case 2: Serial.println("Senzorul ramane blocat pe LOW."); break;
        case 3: Serial.println("Senzorul ramane blocat pe HIGH."); break;
        case 4: Serial.println("Timeout la debutul bitului (Linii zgomotoase)."); break;
        case 5: Serial.println("Timeout la masurarea starii HIGH."); break;
        case 6: 
          Serial.print("Checksum incorect! Raw Data: ");
          for(uint8_t k=0; k<5; k++) { Serial.print(dht_date[k]); Serial.print(" "); }
          Serial.println();
          break;
      }
    }
    }

    
  }
}