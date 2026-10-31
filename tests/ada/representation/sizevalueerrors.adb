procedure SizeValueErrors is
    type Value is range 0 .. 100;
    Count : Integer := 8;
    Object : Value;
    for Value'Size use 0;
    for Value'Size use -8;
    for Value'Size use 7;
    for Value'Size use 24;
    for Value'Size use 72;
    for Value'Size use 8.0;
    for Value'Size use True;
    for Value'Size use Count;
    for Object'Size use 8;
    for Missing'Size use 8;
    for Integer'Size use 8;
begin
    null;
end SizeValueErrors;
