
//Judul   : Program Relay dengan Display info, Web Server, Dan Blynk
//Pencipta: Anjas Amar Pradana
//Drive   : ESP8266
//Dibuat  : 20/04/2022

// Impor perpustakaan yang diperlukan
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
// ID Template, Nama Perangkat, dan Token Otentikasi disediakan oleh Blynk.Cloud
// Lihat tab Info Perangkat, atau pengaturan Template
#define BLYNK_TEMPLATE_ID           "Tempalte ID Anda"
#define BLYNK_DEVICE_NAME           "Device Anda"
#define BLYNK_AUTH_TOKEN            "Token Anda"
// Komentari ini untuk menonaktifkan cetakan dan menghemat ruang
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
 
// atur jumlah kolom dan baris LCD
int lcdColumns = 16;
int lcdRows = 2;

// atur alamat LCD, jumlah kolom dan baris
// jika Anda tidak tahu alamat tampilan Anda, jalankan sketsa pemindai I2C
// abaikan jika error terjadi saat kompiler, yang mengatakan tidak support untuk jenis board anda selain AVR,
// ini saya menggunakan perpustakaan dan mengubahnya di properti perpustakaan lalu menambahkan perangkat modul saya.
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 
 

// Setel ke true untuk mendefinisikan Relay sebagai Biasanya Terbuka (NO)
#define RELAY_NO    true

// Setel jumlah relai
#define NUM_RELAYS  5

// Tetapkan setiap GPIO ke relai
int relayGPIOs[NUM_RELAYS] = {14, 12, 13, 15, 02};

// Ganti dengan kredensial jaringan Anda
const char* ssid = "SSID Wifi Anda";
const char* password = "Password Wifi Anda";

// untuk parameter input status dan relay
const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

// untuk autentifikasi Blynk Cloud
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;

// Fungsi ini dipanggil setiap kali status Virtual Pin 0 berubah
BLYNK_WRITE(V0)
{
  // Tetapkan nilai yang masuk dari pin V0 ke variabel
  int value = param.asInt();

  // Perbarui status
  Blynk.virtualWrite(V1, value);
}

// Fungsi ini dipanggil setiap kali perangkat terhubung ke Blynk.Cloud
BLYNK_CONNECTED()
{
  // Ubah pesan Tombol Tautan Web menjadi "Selamat!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// Buat objek AsyncWebServer pada port 80
AsyncWebServer server(80);

// bagian halaman html atau front-end
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Kontrol IOT Saklar Relay</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 800px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Risky Wuelek - ESP8266 <br/>Web Server IoT Controller</h2><hr/>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
  xhr.send();
}</script>
</body>
</html>
)rawliteral";

// Menggantikan placeholder dengan bagian tombol di halaman web Anda
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      
      // Fungsi Untuk Tombol On / Off
      buttons+= "<h4>Saklar Relay #" + String(i) + " - GPIO Ke " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label><hr/>";
    }
    return buttons;
  }
  return String();
}
String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "";
    }
    else {
      return "checked";
    }
  }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "checked";
    }
    else {
      return "";
    }
  }
  return "";
}

// Fungsi ini mengirimkan uptime Arduino setiap detik ke Virtual Pin 2.
void myTimerEvent()
{
  // Anda dapat mengirim nilai apa pun kapan saja.
  // Tolong jangan mengirim lebih dari 10 nilai per detik.
  Blynk.virtualWrite(V2, millis() / 1000);
}

// bagian fungsi setup
void setup(){

// Debug console
  Serial.begin(115200);

  Blynk.begin(auth, ssid, pass);
  // Anda juga dapat menentukan server:
  //Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192.168.1.100), 8080);

  // Fungsi setup yang akan dipanggil setiap detik
  timer.setInterval(1000L, myTimerEvent);

// initialize LCD
  lcd.init();
  // nyalakan lampu latar LCD                      
  lcd.backlight();
  
  // Port serial untuk keperluan debugging
  Serial.begin(115200);

  // Setel semua relai ke nonaktif saat program dimulai - jika disetel ke Biasanya Terbuka (NO), relai mati ketika Anda mengatur relai ke HIGH
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], HIGH);
    }
    else{
      digitalWrite(relayGPIOs[i-1], LOW);
    }
  }
  
  // Menghubungkan ke Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan Ke Jaringan WiFi..");
  }

  // Cetak ESP8266 Alamat IP Lokal
  Serial.println(WiFi.localIP());

  // Rute untuk root / halaman web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Kirim permintaan GET ke <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        Serial.print("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
      }
      else{
        Serial.print("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  // Start server
  server.begin();
}


String messageStatic = "KSRM:";
String messageToScroll = "Kontrol Saklar Relay Monitor";
String messageInfoPortNonAktif = "Tidak/Belum Digunakan";

// Fungsi untuk text skrol Judul projek
// Fungsi menerima argumen berikut::
// row: nomor baris tempat teks akan ditampilkan
// message: pesan ke skrol
// delayTime: penundaan antara setiap perpindahan karakter
// lcdColumns: angka dari kolom dari LCD
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// bagian looping program
void loop() {

  Blynk.run();
  timer.run();
  // Anda dapat menyuntikkan kode Anda sendiri atau menggabungkannya dengan sketsa lain.
  // untuk menghindari fungsi delay()!

  // set kursor ke kolom pertama (0), row pertama(0)
  // untuk menampilkan info atau judul sistem
  lcd.setCursor(0, 0);
  // print pesan statis
  lcd.print(messageStatic);
  // print pesan skrol
  scrollText(0, messageToScroll, 260, lcdColumns);
  
  // set kursor ke kolom pertama(0), row kedua (1)
  // untuk menampilkan status relay atau status perangkat lainya anda bisa menambahkannya nanti
  lcd.setCursor(0,1);
  lcd.print("Status R1:");
  delay(7000);
  lcd.clear();
  
  lcd.setCursor(0,1);
  lcd.print("Status R2:");
  delay(7000);
  lcd.clear();
  
  lcd.setCursor(0,1);
  lcd.print("Status R3:");
  delay(7000);
  lcd.clear();

  lcd.setCursor(0,1);
  lcd.print("Status R4:");
  delay(7000);
  lcd.clear();

  lcd.setCursor(0,1);
  lcd.print("Status R5: ");
  // print pesan info skrol
  scrollText(1, messageInfoPortNonAktif, 240, lcdColumns);
  delay(7000);
  lcd.clear();
}
