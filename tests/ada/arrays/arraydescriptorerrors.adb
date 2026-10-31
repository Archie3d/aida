procedure ArrayDescriptorErrors is
    type Full_Span is array (Long_Integer range Long_Integer'First .. Long_Integer'Last) of Character;
    type Byte_Overflow is array (Long_Integer range 1 .. Long_Integer'Last) of Integer;
    type Matrix_Overflow is array (Long_Integer range 1 .. 5_000_000_000,
        Long_Integer range 1 .. 5_000_000_000) of Integer;
    type Bits is array (Long_Integer range 1 .. Long_Integer'Last) of Character;
    Bad_Bits : Long_Integer := Bits'Size;
    type Too_Wide_Record is record
        Prefix : Integer;
        Data : Bits;
    end record;
begin
    null;
end ArrayDescriptorErrors;
