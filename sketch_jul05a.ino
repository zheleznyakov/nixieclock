//Подключаем библиотеки для работы часов реального времени

#include <DS3231.h>
#include <Wire.h>

// выводы для дешифратора
  int out1 = A3;
  int out2 = A1;
  int out4 = A0;
  int out8 = A2;
// выводы для транзисторных ключей
  int key1= 4;
  int key2 = 6;
  int key3 = 7;
  int key4 = 8;

  int led1=11;// вывод светодиода
  int keyb=A6;// пин для клавиатуры


  int keynum=0; //номер нажатой кнопки
  int mode =0; //0 - отображение текущего времени 1- установка времени
  int currentdigit = 0; //0 - установка часов 1- установка минут
  bool blinkflag = 0; // флаг для мигания цифр при установке времени
  bool timeout=false; // таймаут после нажатия кнопки (для устраниния дребезга)
  unsigned long startTime; //для мигания светодиода
  unsigned long startTime2; // для опроса нажатых кнопок
  int sec; //переменная увеличивается каждые 500мс на единицу. Нужна для мигания секундных светодиодов, бывает в двух значениях 0 - светодиоды не горят, 1 - горят, если больше 1, то сбрысывается в 0

  // переменные, необходимые для работы ds3231
  DS3231 Clock;
  bool Century=false;
  bool h12;
  bool PM;
  byte ADay, AHour, AMinute, ASecond, ABits;
  bool ADy, A12h, Apm;

  //определяем глобальные переменные для различных параметров часов
  int second,minute,hour,date,month,year,temperature;


void setup() {
  // put your setup code here, to run once:
  // задаем частоту ШИМ на 9 выводе 30кГц
  TCCR1B=TCCR1B&0b11111000|0x01;
  analogWrite(9,130);

    // Start the I2C interface
  Wire.begin();
  // Start the serial interface
  Serial.begin(9600);
  
  //задаем режим работы выходов микроконтроллера
  pinMode(out1,OUTPUT);
  pinMode(out2,OUTPUT);
  pinMode(out4,OUTPUT);
  pinMode(out8,OUTPUT);

  pinMode(key1,OUTPUT);
  pinMode(key2,OUTPUT);
  pinMode(key3,OUTPUT);
  pinMode(key4,OUTPUT);

  pinMode(led1,OUTPUT);

  startTime = millis();
  startTime2=startTime;
  sec = 0;
  // считываем текущее вермя
  ReadDS3231();
}

void ReadDS3231()   //функция считывает в глобальные переменные minute и hour текущие значения
{
  minute=Clock.getMinute();
  hour=Clock.getHour(h12, PM);
}


void loop() {
  // put your main code here, to run repeatedly:
  
  int digits[3]; // массив для текущего значения времени на четыре цифры
  int keyval= analogRead(keyb); // считываем значение с клавиатуры
  
  unsigned long currentTime = millis(); //текущее время с момента запуска рограммы
   if (currentTime>=(startTime2+200)) // если прошло 200мс посмотрим какая кнопка была нажата
   {
      timeout=false;
     //проверим нажатие кнопок
      if (keynum==3) // кнопка переключения между режимами "время" и "настройка"
      {
        timeout=true; // запрещаем считывать нажатия кнопок
        if (mode==0) mode=1; // если отображалось время, переходим в режим настроек
        else
          if (mode==1) mode=0; //если был режим настроек, переключаемся на текущее время
      }
      if (keynum==2) // кнопка переключения между часами и минутами
      {
        timeout=true; // запрещаем считывать нажатия кнопок
        // currentdigit либо 0 - настройка часов, либо 1 - настройка минут
        currentdigit++; 
        if (currentdigit==2) currentdigit=0;
      }
      if (keynum==1) // кнопка увеличения количества часов или минут(при настройке)
      {
        blinkflag=true; // прекращаем мигать цифрами
        startTime=millis();
        timeout=true;  // запрещаем считывать нажатия кнопок
        if (currentdigit==0) // если меняем часы
        {
          if (hour<=22) hour++;
          else
            hour=0;
          Clock.setHour(hour);
        }
        if (currentdigit==1) // если меняем минуты
        {
          if (minute<=58) minute++;
          else 
            minute=0;
         Clock.setMinute(minute);//Set the minute 
         Clock.setSecond(0);
        }
      }
      keynum=0; //сбрасываем нажатую кнопку
      startTime2=millis();
   }

  if (currentTime>=(startTime+500)) // если прошло 500мс
  {
    blinkflag=!blinkflag; // инвертируем флаг мигающей цифры
      
    if(sec<1) // светодиоды не горят
    {
      // зажечь светодиоды
      digitalWrite(led1,HIGH);
      Serial.println(blinkflag);
      
    }
    if(sec>=1) // светодиоды горят
    {
      // потушить светодиоды
      digitalWrite(led1,LOW);
    }
    startTime = currentTime;
    ReadDS3231(); 
    sec = (sec+1);
    if(sec>=2) sec=0;

  }
    digits[0] = hour/10;
    digits[1] = hour%10;
    digits[2] = minute/10;
    digits[3] = minute%10;

    show(digits); // вывести цифры на дисплей

  //проверяем нажатые кнопки
  if(!timeout) //если разрешено, считываем нажатую кнопку
  {
    /*keyval - значение функции analogread 
     * если нажата первая кнопка, то принимает значение около 200
     * если нажата вторая кнопка то значение около 700
     * если третяя кнопка, то значение будет около 1000
     * эти значения зависят от выбранных резисторов в клавиатуре
    */
    if (keyval>150 && keyval<400) keynum=1;
    if (keyval>700 && keyval<900) keynum=2;
    if (keyval>960) keynum=3;
  }
}
void show(int a[])
{
    //выведем цифру a[0] на первый индикатор
  setNumber(a[0]);
  if (!(mode==1&&currentdigit==0&&blinkflag==false)) //если мы в режиме настройки и происходит настройка часов, то в первая цифра будет мигать
  {
    digitalWrite(key1,HIGH);
    delay(2);
    //потушим первый индикатор
    digitalWrite(key1,LOW);
    delay(1);
  }
  
  //цифра a[1] на второй индикатор
  setNumber(a[1]);
  if (!(mode==1&&currentdigit==0&&blinkflag==false))
  {
  digitalWrite(key2,HIGH);
  delay(2);
  //потушим второй индикатор
  digitalWrite(key2,LOW);
  delay(1);
  }

  //цифра a[2] на третий индикатор
  setNumber(a[2]);
  if (!(mode==1&&currentdigit==1&&blinkflag==false))
  {
  digitalWrite(key3,HIGH);
  delay(2);
  //потушим третий индикатор
  digitalWrite(key3,LOW);
  delay(1);
  }

  //выведем цифру a[3] на четвертый индикатор
  setNumber(a[3]);
  if (!(mode==1&&currentdigit==1&&blinkflag==false))
  {
  digitalWrite(key4,HIGH);
  delay(2);
  //потушим четвертый индикатор
  digitalWrite(key4,LOW);
  delay(1);
  }
}
// передача цифры на дешифратор
void setNumber(int num) 
{
  switch (num)
  {
    case 0:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out4,LOW);
    digitalWrite (out8,LOW);
    break;
    case 1:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out4,LOW);
    digitalWrite (out8,LOW);
    break;
    case 2:
    digitalWrite (out1,LOW);
    digitalWrite (out2,HIGH);
    digitalWrite (out4,LOW);
    digitalWrite (out8,LOW);
    break;
    case 3:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,HIGH);
    digitalWrite (out4,LOW);
    digitalWrite (out8,LOW);
    break;
    case 4:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out4,HIGH);
    digitalWrite (out8,LOW);
    break;
    case 5:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out4,HIGH);
    digitalWrite (out8,LOW);
    break;
    case 6:
    digitalWrite (out1,LOW);
    digitalWrite (out2,HIGH);
    digitalWrite (out4,HIGH);
    digitalWrite (out8,LOW);
    break;
    case 7:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,HIGH);
    digitalWrite (out4,HIGH);
    digitalWrite (out8,LOW);
    break;
    case 8:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out4,LOW);
    digitalWrite (out8,HIGH);
    break;
    case 9:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out4,LOW);
    digitalWrite (out8,HIGH);
    break;
  }
}

