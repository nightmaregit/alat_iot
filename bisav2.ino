#include <WiFi.h>
#include <FirebaseClient.h>
//#include "ExampleFunctions.h" // Provides the functions used in the examples.
#include <Servo.h>
#include "DHTesp.h"
 const int espLed = 2;
 const int ledPin1 = 4;
 const int ledPin2 = 5;
 const int ledPin3 = 18;
 const int ledPin4 = 19;
 int servoPin = 21;
 int DHT_PIN=22;
 Servo servo1;
 DHTesp dhtSensor;

#define WIFI_SSID "Galaxy A30s8D73j"
#define WIFI_PASSWORD "12345678901"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSyDRyw-2rbE8A3n_lqR25fbD3ASOyfIT7uM"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "coba@gmail.com"
#define USER_PASSWORD "qwer1234"
#define DATABASE_URL "https://web-iot-8893b-default-rtdb.firebaseio.com"

void processData(AsyncResult &aResult);

#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client, stream_ssl_client;
//SSL_CLIENT ssl_client, stream_ssl_client;

// This uses built-in core WiFi/Ethernet for network connection.
// See examples/App/NetworkInterfaces for more network examples.
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client), streamClient(stream_ssl_client);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) */);
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult streamResult;

unsigned long ms = 0;

bool lampuDapurStatus = false;
bool lampuMakanStatus = false;
bool kipasKamarStatus = false;
int kecepatankipas = 0;
bool taskComplete = false;

void setup()
{
    Serial.begin(115200);
    pinMode(espLed, OUTPUT);
    pinMode(ledPin1, OUTPUT);
    pinMode(ledPin2, OUTPUT); 
    pinMode(ledPin3, OUTPUT);
    pinMode(ledPin4, OUTPUT);
    servo1.attach(servoPin);
    dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        digitalWrite(espLed, LOW );
        delay(1500);
        digitalWrite(espLed, HIGH );
        delay(1500);
        digitalWrite(espLed, LOW );
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    digitalWrite(espLed, HIGH );
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl_client.setInsecure();
    stream_ssl_client.setInsecure();
    //set_ssl_client_insecure_and_buffer(ssl_client);
    //set_ssl_client_insecure_and_buffer(stream_ssl_client);

    Serial.println("Initializing app...");
    initializeApp(aClient, app, getAuth(user_auth));

    // Or intialize the app and wait.
    // initializeApp(aClient, app, getAuth(user_auth), 120 * 1000, auth_debug_print);

    app.getApp<RealtimeDatabase>(Database);

    Database.url(DATABASE_URL);

    streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");

    Database.get(streamClient, "/", processData, true /* SSE mode (HTTP Streaming) */, "streamTask");

}

void loop()
{
    // To maintain the authentication and async tasks
    app.loop();

    if (app.ready() && !taskComplete)
    {
        taskComplete = true;
        getSekali();
    }


    if (app.ready() && millis() - ms > 2500)
    {
        ms = millis();
        TempAndHumidity data = dhtSensor.getTempAndHumidity();
        float temperature = data.temperature;
        float humidity = data.humidity;

        if(!isnan(temperature) && !isnan(humidity)){
        Database.set(aClient, "/dht22/temperature", temperature, processData, "setTemperatureTask");
        Database.set(aClient, "/dht22/humidity", humidity, processData,"setHumidityTask");
        }else{
           Serial.println("Temperature dan Humidity memiliki nilai tidak valid.");          
        }
    }
    // For async call with AsyncResult.
    // processData(streamResult);
}

void processData(AsyncResult &aResult)
{
    // Exits when no result available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream())
        {
            String path = RTDB.dataPath();
            bool status = RTDB.to<bool>();
            int kecepatan = RTDB.to<int>();
            // Firebase.printf("path: %s\n", RTDB.dataPath().c_str());

            if (path == "/Lampu/dapur")
            {
                lampuDapurStatus = status;
                Firebase.printf("--------------------\n");
                Firebase.printf(lampuDapurStatus ? "Lampu Dapur: ON \n" : "Lampu Dapur: OFF \n");
                digitalWrite(ledPin1, lampuDapurStatus ? HIGH : LOW );
            }
            else if (path == "/Lampu/makan")
            {
                lampuMakanStatus = status;
                Firebase.printf("-------------------- \n");
                Firebase.printf(lampuMakanStatus ? "Lampu Makan: ON \n" : "Lampu Makan: OFF \n");
                digitalWrite(ledPin2, lampuMakanStatus ? HIGH : LOW );
            }
            else if (path == "/Kipas/kamar")
            {
                kipasKamarStatus = status;
                Firebase.printf("--------------------\n");
                Firebase.printf(kipasKamarStatus ? "Kipas Kamar: ON \n" : "Kipas Kamar: OFF \n");
                digitalWrite(ledPin3, kipasKamarStatus ? HIGH : LOW );
            }
             else if (path == "/Kipas/kecepatankamar")
            {
                kecepatankipas = kecepatan;
                Firebase.printf("-------------------- \n");
                servo1.write(kecepatankipas);
                Firebase.printf("kecepatan kipas : %d\n", kecepatankipas);
            }
        }
        else
        {
            Serial.println("----------------------------");
            Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        }
        // Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
    }
}

void getSekali(){
    Serial.println("Getting the bool value dapur... ");
    bool dapur = Database.get<bool>(aClient, "/Lampu/dapur");
    if (dapur) {
    digitalWrite(ledPin1, dapur ? HIGH : LOW);
    Firebase.printf("Lampu Dapur: ON \n");
    }else{
    digitalWrite(ledPin1, dapur);
    Firebase.printf("Lampu Dapur: OFF \n");
    }

    Serial.println("Getting the bool value makan... ");
    bool makan = Database.get<bool>(aClient, "/Lampu/makan");
    if (makan) {
    digitalWrite(ledPin2, makan ? HIGH : LOW);
    Firebase.printf("Lampu makan: ON \n");
    }else{
    digitalWrite(ledPin2, makan);
    Firebase.printf("Lampu makan: OFF \n");
    }

    Serial.println("Getting the bool value kipas... ");
    bool kipas = Database.get<bool>(aClient, "/Kipas/kamar");
    if (kipas) {
    digitalWrite(ledPin3, kipas ? HIGH : LOW);
    Firebase.printf("Lampu kipas: ON \n");
    }else{
    digitalWrite(ledPin3, kipas);
    Firebase.printf("Lampu kipas: OFF \n");
    }

    Serial.println("setting the int value kipas... ");
    int speed = Database.get<int>(aClient, "/Kipas/kecepatankamar");
    servo1.write(speed);
    Firebase.printf("kecepatan kipas : %d\n", speed);
    
}
