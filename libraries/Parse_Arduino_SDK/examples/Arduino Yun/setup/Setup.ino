#include <Bridge.h>

String revision = "1.0.3-1_ar71xx";
String location = "https://raw.githubusercontent.com/ParsePlatform/parse-embedded-sdks/1.0.3/yun/linux_package/";

void downloadPackage(String file) {
  Serial.println("Download: " + location + file + revision + ".ipk");
  Process p;
  p.begin("curl");
  p.addParameter("--stderr");
  p.addParameter("-");
  p.addParameter("-#");
  p.addParameter("-s");
  p.addParameter("-S");
  p.addParameter("-k");
  p.addParameter("-o");
  p.addParameter("/tmp/" + file + revision + ".ipk");
  p.addParameter(location + file + revision + ".ipk");
  p.run();
  while (p.available()) {
    Serial.print((char)p.read());
  }
}

void installPackage(String file) {
  Serial.println("Install: /tmp/" + file + revision + ".ipk");
  Process p;
  p.begin("opkg");
  p.addParameter("install");
  p.addParameter("--force-reinstall");
  p.addParameter("--force-downgrade");
  p.addParameter("/tmp/" + file + revision + ".ipk");
  p.run();
  while(p.available()) {
    Serial.print((char)p.read());
  }
}

void setup() {
  Bridge.begin();
  Serial.begin(115200);

  while(!Serial);

  Serial.println("Downloading packages");
  downloadPackage("parse-embedded_");
  downloadPackage("parse-embedded-yun_");
  Serial.println("Installing packages");
  installPackage("parse-embedded_");
  installPackage("parse-embedded-yun_");

  Serial.println("\nDone.");
}

void loop() {

}
