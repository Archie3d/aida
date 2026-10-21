procedure ArrayFrameErrors is
    type Half is array (Long_Integer range 1 .. Long_Integer'Last / 2 + 1) of Character;
    type Holder is record
        Data : Half;
    end record;
    A : Holder;
    function Read return Boolean is
    begin
        return A.Data (1) = 'a';
    end Read;
begin
    if Read then
        null;
    end if;
end ArrayFrameErrors;
