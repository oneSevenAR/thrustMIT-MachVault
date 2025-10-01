void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:

}

void loop() {
  int sv=analogRead(A0);
  Serial.println(sv);
  delay(50);
  // put your main code here, to run repeatedly:

}
