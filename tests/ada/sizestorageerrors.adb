with Ada.Finalization;
procedure SizeStorageErrors is
    Limit : Integer := 10;
    subtype Dynamic is Long_Integer range 0 .. Long_Integer (Limit);
    for Dynamic'Size use 8;
    subtype Empty is Integer range 1 .. 0;
    for Empty'Size use 8;
    subtype Too_Wide is Integer range -129 .. 127;
    for Too_Wide'Size use 8;
    subtype Late is Integer range -128 .. 127;
    Object : Late;
    for Late'Size use 8;
    type Pair is record
        A, B : Integer;
    end record;
    for Pair'Size use 32;
    for Pair'Size use 72;
    type Values is array (1 .. 3) of Integer;
    for Values'Size use 64;
    for Values'Size use 104;
    type Open_Values is array (Integer range <>) of Integer;
    for Open_Values'Size use 128;
    subtype Runtime_Values is Open_Values (1 .. Limit);
    for Runtime_Values'Size use 128;
    type Late_Record is record
        A : Integer;
    end record;
    Item : Late_Record;
    for Late_Record'Size use 64;
    type Controlled is new Ada.Finalization.Controlled with null record;
    type Holding is record
        Value : Controlled;
    end record;
    for Holding'Size use 128;
    task type Worker;
    for Worker'Size use 128;
    task body Worker is
    begin
        null;
    end Worker;
begin
    null;
end SizeStorageErrors;
