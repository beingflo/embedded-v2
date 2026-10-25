void setup()
{
  pinMode(A5, OUTPUT);
}

void loop()
{
  digitalWrite(A5, HIGH);
  delay(1000);
  digitalWrite(A5, LOW);
  delay(1000);
}
