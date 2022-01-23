# FolderAutoCompiler
A tool which "compiles" multiple folders into one for better management

# How to use it
Create a file in the same folder as the exe named `repoconfig.txt`. 
When you run the program it will read this config file.

# Config syntax and commands
The config has the following available sections (case insensitive):
 - **pre**        `List of commands to be executed before any operation`
 - **repo_name**  `One line which contains the name of the folder where all should be compiled`
 - **list**       `A list of folders to be copied into the <repo_name> folder`
 - **post**       `List of commands to be executed after compillation`
 
 The config format:
 
```
<section>:
<one line or multiple lines>
<next_section>:
<one line or multiple lines>
```
 
 Examples:
 
```
repo_name:
Repo
list:
"C:\Users\Me\Documents\Arduino\SolderIron" "Arduino"
"C:\Users\Me\Documents\KiCad Projects\SolderIron" "Schematics"
```
 
```
repo_name:
Repo
list:
"C:\Users\Me\Documents\Arduino\ESP32_AutoGradina" "ESP32_AutoGradina"
"C:\Users\Me\Documents\KiCad Projects\Autogradina" "Schematica"

"C:\Users\Me\Documents\Arduino\libraries\FixedString" "libraries\FixedString"
"C:\Users\Me\Documents\Arduino\libraries\ESP32_Timer" "libraries\ESP32_Timer"
"C:\Users\Me\Documents\Arduino\libraries\JSON" "libraries\JSON"
"C:\Users\Me\Documents\Arduino\libraries\RTClib" "libraries\RTClib"
"C:\Users\Me\Documents\Arduino\libraries\DynamicDebug" "libraries\DynamicDebug"

post:
fart.exe -r *.ino "\"RANDOMLY_GENERATED_WIFI_SSID_HERE\"" "\"YOUR SSID\""
fart.exe -r *.ino "RANDOMLY_GENERATED_WIFI_PASS_HERE" "YOUR PASSWORD"
fart.exe -r *.ino "RANDOMLY_GENERATED_PASS_HERE" "YOUR ARDUINO OTA PASSWORD"
```
