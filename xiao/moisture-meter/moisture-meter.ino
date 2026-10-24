void setup()
{
  Serial.begin(115200);
  pinMode(D6, OUTPUT);
}

void loop()
{
  digitalWrite(D6, HIGH);
  delay(50);
  int moisture = analogRead(A1);
  digitalWrite(D6, LOW);
  Serial.println(moisture);
  delay(2000);
}