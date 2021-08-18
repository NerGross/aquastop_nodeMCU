#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Bounce2.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#define cold_close  D0          // D0 V0
#define cold_open  D1           // D1 V1
#define hot_close  D2           // D2 V2
#define hot_open  D3            // D3 V3
#define sensor D4               // D4 
#define button_close  D5        // D5
#define button_open  D6         // D6
#define led_working  D7         // D7
#define led_alarm  D8           // D8

bool bclose;
bool bopen;
bool state_sensor;
bool state_cold_close;
bool state_cold_open;
bool state_hot_close;
bool state_hot_open;
bool flag = false;

int bot_RequestDelay = 43200000; //переменная для оповещения раз в 12 часов (43200000)
unsigned long bot_lasttime;

int bot_RequestDelayAlarm = 1800000; //переменная для оповещения раз в 30 минут (1800000)
unsigned long bot_lasttimeAlarm;

char auth[] = "##########"; //универсальный ключ ID Blynk
char ssid[] = "##########";                         //логин WIFI
char pass[] = "##########";                         //пароль WIFI
char BOTtoken[] = "##########"; //универсальный ключ ID Telegram
char my_chat_id[] = "##########";  //универсальный ключ ID Telegram

WiFiClientSecure client; //Создаем новый клиент Wi-Fi
UniversalTelegramBot bot(BOTtoken, client);

Bounce debouncerO = Bounce();
Bounce debouncerC = Bounce();

BLYNK_WRITE(V0) //функция, отслеживающая изменение виртуального пина 0 (закр. хол воды)
{
  bool pinValue0 = param.asInt(); //переменная текущего состояния виртуального пина
  digitalWrite(cold_close, pinValue0); //задаем значение на физическом пине NodeMcu D0 равное значению виртуального пина 0
}

BLYNK_WRITE(V1) //функция, отслеживающая изменение виртуального пина 1 (открытие. хол воды)
{
  bool pinValue1 = param.asInt(); //переменная текущего состояния виртуального пина
  digitalWrite(cold_open, pinValue1); //задаем значение на физическом пине NodeMcu D1 равное значению виртуального пина 1
}

BLYNK_WRITE(V2) //функция, отслеживающая изменение виртуального пина 2 (закр. гор воды)
{
  bool pinValue2 = param.asInt(); //переменная текущего состояния виртуального пина
  digitalWrite(hot_close, pinValue2); //задаем значение на физическом пине NodeMcu D2 равное значению виртуального пина 2
}

BLYNK_WRITE(V3) //функция, отслеживающая изменение виртуального пина 3 (откр. гор воды)
{
  bool pinValue3 = param.asInt(); //переменная текущего состояния виртуального пина
  digitalWrite(hot_open, pinValue3); //задаем значение на физическом пине NodeMcu D3 равное значению виртуального пина 3
}

WidgetLED led1 (V10); //виртуальный светодиод сенсор

void setup()
{
  pinMode(cold_close, OUTPUT);  //объявляем D0 "выходным" пином cold_close
  pinMode(hot_close, OUTPUT);   //объявляем D1 "выходным" пином cold_open
  pinMode(cold_open, OUTPUT);   //объявляем D2 "выходным" пином hot_close
  pinMode(hot_open, OUTPUT);    //объявляем D3 "выходным" пином hot_open
  pinMode(sensor, INPUT);       //объявляем D4 "входным" пином sensor
  pinMode(button_close, INPUT); //объявляем D5 "входным" пином button_close
  pinMode(button_open, INPUT);  //объявляем D6 "входным" пином button_open
  pinMode(led_working, OUTPUT); //объявляем D7 "выходным" пином led_working
  pinMode(led_alarm, OUTPUT);   //объявляем D8 "выходным" пином led_alarm

  // Даем бибилотеке знать, к какому пину мы подключили кнопку
  debouncerO.attach(button_open);
  debouncerO.interval(5); // Интервал, в течение которого мы не буем получать значения с пина
  debouncerC.attach(button_close);
  debouncerC.interval(5); // Интервал, в течение которого мы не буем получать значения с пина

  Serial.begin(115200); //открываем серийный порт, чтобы видеть как проходит подключение к серверу
  client.setInsecure();
  Blynk.begin(auth, ssid, pass); //авторизируемся на сервере
  bot.sendMessage(my_chat_id, "Bot started up", "");
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)//проверяем доступные сообщения
  {
    String chat_id = String(bot.messages[i].chat_id); //Определяем, кто отправил сообщение.
    if (chat_id != my_chat_id)
    {
      bot.sendMessage(my_chat_id, "Unauthorized user", "");//Определяем, что кто-то в чате.
      continue;
    }
    String text = bot.messages[i].text; //если от авторизованного пользователя, то сохраняем в текстовой переменной
    String from_name = bot.messages[i].from_name; //сохраняем имя отправителя

    if (text == "/options")
    {
      String keyboardJson = "[[\"/water_on\", \"/water_off\"],[\"/state\"]]";
      bot.sendMessageWithReplyKeyboard(my_chat_id, "Choose from one of the following options", "", keyboardJson, true);
    }
    else if (text == "/state")
    {
      state_sensor = digitalRead(sensor); //считываем состояние датчика
      if (!flag)
      {
        bot.sendMessage(my_chat_id, "state ОК, water is open");
      }
      else
      {
        if (state_sensor)
        {
          bot.sendMessage(my_chat_id, "water is close, sensor without alarm");
        }
        else
        {
          bot.sendMessage(my_chat_id, "water is close, sensor in alarm");
        }
      }
    }
    else if (text == "/water_on" )
    {
      digitalWrite(cold_open, 1); //открытие хол воды
      digitalWrite(hot_open, 1);  //открытие гор воды
      digitalWrite(led_alarm, 0); //выключаем светодиод аварии
      flag = false;
      bot.sendMessage(my_chat_id, "water is open");
    }
    else if (text == "/water_off" )
    {
      digitalWrite(led_alarm, 1); //включаем светодиод аварии
      digitalWrite(cold_close, 1); //закрываем хол воды
      digitalWrite(hot_close, 1); //закрываем гор воды
      flag = true;
      bot.sendMessage(my_chat_id, "water is close");
    }
    else
    {
      bot.sendMessage(my_chat_id, "command not found");
    }
  }
}

void loop()
{
  digitalWrite(led_working, 1); //включаем светодиод состояния

  // проверка присланных сообщений в телеграмм
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages)
  {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }

  //оповещение раз в 12 часов
  if (millis() > bot_lasttime + bot_RequestDelay)
  {
    state_sensor = digitalRead(sensor); //считываем состояние датчика
    if (state_sensor)
    {
      bot.sendMessage(my_chat_id, "ОК, water is open");
    }
    else
    {
      bot.sendMessage(my_chat_id, "OK, water is close");
    }
    bot_lasttime = millis();
  }
  Blynk.run();                                //запускаем работу blynk.
  state_cold_close = digitalRead(cold_close); //считываем состояние датчика
  state_cold_open = digitalRead(cold_open);   //считываем состояние датчика
  state_hot_close = digitalRead(hot_close);   //считываем состояние датчика
  state_hot_open = digitalRead(hot_open);     //считываем состояние датчика
  if (state_cold_close || state_hot_close) //проверяем состояние напряжения на пинах, для снятия его с них через 30 сек
  {
    digitalWrite(led_working, 0); //включаем светодиод состояния
    delay(15000);                 //  подождать 30 сек
    digitalWrite(cold_close, 0);  //  откл. закрытие хол воды
    digitalWrite(hot_close, 0);   //  откл. закрытие гор воды
    digitalWrite(led_alarm, 1); //включаем светодиод состояния
    if (!flag) //смотрим что приходит true
    {
      bot.sendMessage(my_chat_id, "the water is close from BLYNK");
      flag = true;
    }
    else
    {
      flag = true;
    }
  }
  else if (state_cold_open || state_hot_open) //проверяем состояние напряжения на пинах, для снятия его с них через 30 сек
  {
    digitalWrite(led_working, 0); //включаем светодиод состояния
    delay(15000);                 //  подождать 30 сек
    digitalWrite(cold_open, 0);  //  откл. закрытие хол воды
    digitalWrite(hot_open, 0);   //  откл. закрытие гор воды
    digitalWrite(led_alarm, 0); //включаем светодиод состояния
    if (flag) //смотрим что приходит false
    {
      bot.sendMessage(my_chat_id, "the water is open from BLYNK");
      flag = false;
    }
    else
    {
      flag = false;
    }
  }
  else
  {
    state_sensor = digitalRead(sensor); //считываем состояние датчика
    if (state_sensor)               //без аварии
    {
      led1.off();
      bopen = debouncerO.update();  //обновляем состояние нажатия кнопки открыть
      bclose = debouncerC.update(); //обновляем состояние нажатия кнопки закрыть
      if (bopen)                    //кнопка открытия нажата
      {
        digitalWrite(cold_open, 1); //открытие хол воды
        digitalWrite(hot_open, 1);  //открытие гор воды
        digitalWrite(led_alarm, 0); //выключаем светодиод аварии
        flag = false;
        bot.sendMessage(my_chat_id, "the water is open from the button");
      }
      else if (bclose)              //кнопка закрытия нажата
      {
        digitalWrite(led_alarm, 1); //включаем светодиод аварии
        digitalWrite(cold_close, 1); //закрываем хол воды
        digitalWrite(hot_close, 1); //закрываем гор воды
        flag = true;
        bot.sendMessage(my_chat_id, "the water is closed from the button");
      }
    }
    else
    {
      led1.on();
      flag = true;
      bot.sendMessage(my_chat_id, "alarm, water is close");
      digitalWrite(led_alarm, 1); //включаем светодиод аварии
      // 2 раза даем команду на закрытие с перерывом в 1 минута
      for (int i = 0; i < 2; i++)
      {
        digitalWrite(cold_close, 1);  //закрываем хол воды
        digitalWrite(hot_close, 1);   //закрываем гор воды
        //отключаем напряжение
        delay(15000);                 // подождать 30 сек
        digitalWrite(cold_close, 0);  // откл. закрытие хол воды
        digitalWrite(hot_close, 0);   // откл. закрытие гор воды
        delay(15000);                 // подождать 30 сек
      }
      //уходим в бесконечный цикл до нажатия кнопки открытия или открыия через приложение
      do
      {
        state_cold_open = digitalRead(cold_open);    //считываем состояние датчика
        state_hot_open = digitalRead(hot_open);      //считываем состояние датчика
        state_sensor = digitalRead(sensor); //считываем состояние датчика
        if (!state_sensor)  //в аварии
        {
          //оповещение раз в 30 минут
          if (millis() > bot_lasttimeAlarm + bot_RequestDelayAlarm)
          {
            bot.sendMessage(my_chat_id, "alarm, water is close, sensor in alarm");
            bot_lasttimeAlarm = millis();
          }
          led1.on();
          digitalWrite(led_working, 1);   //выключаем светодиод состояния
          delay(500);
          led1.off();
          digitalWrite(led_working, 0);   //выключаем светодиод состояния
          delay(500);
        }
        else if (state_sensor && (state_cold_open || state_hot_open))
        {
          flag = false;
          bot.sendMessage(my_chat_id, "the water is open from BLYNK");
          break;
        }
        else
        {
          //оповещение раз в 30 минут
          if (millis() > bot_lasttimeAlarm + bot_RequestDelayAlarm)
          {
            bot.sendMessage(my_chat_id, "alarm, water is close, sensor without alarm");
            bot_lasttimeAlarm = millis();
          }
          // проверка присланных сообщений в телеграмм
          int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
          while (numNewMessages)
          {
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
          }
          Blynk.run();                    //запускаем работу blynk в бесконечном цикле
          led1.on();
          digitalWrite(led_working, 1);   //выключаем светодиод состояния
          delay(2000);
          led1.off();
          digitalWrite(led_working, 0);   //выключаем светодиод состояния
          delay(2000);
        }
        bopen = debouncerO.update();    //обновляем состояние нажатия кнопки
      }
      while (!bopen);  //кнопка открытия нажата или открыли через blynk
      flag = false;
    }
  }
}
