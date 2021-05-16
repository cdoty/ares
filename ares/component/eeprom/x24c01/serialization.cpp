auto X24C01::serialize(serializer& s) -> void {
  s(bytes);
  s(clock.lo);
  s(clock.hi);
  s(clock.fall);
  s(clock.rise);
  s(clock.line);
  s(data.lo);
  s(data.hi);
  s(data.fall);
  s(data.rise);
  s(data.line);
  s((u32&)mode);
  s(counter);
  s(address);
  s(input);
  s(output);
}
