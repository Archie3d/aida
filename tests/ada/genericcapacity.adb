with Ada.Text_IO;
procedure GenericCapacity is
    generic
        capacity : Positive;
    package Buffers is
        subtype Count is Natural range 0 .. capacity;
        subtype Text is String (1 .. capacity);
        type Buffer is record
            used : Count := 0;
            data : Text := (others => 'x');
        end record;
        function Make return Buffer;
    end Buffers;
    package body Buffers is
        function Make return Buffer is
            result : Buffer;
        begin
            result.used := capacity;
            return result;
        end Make;
    end Buffers;
    package Two is new Buffers (2);
    package Nine is new Buffers (9);
    a : Two.Buffer := Two.Make;
    b : Nine.Buffer := Nine.Make;
begin
    if a.data /= "xx" or b.data /= "xxxxxxxxx" or a.used /= 2 or b.used /= 9 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("genericcapacity: passed");
end GenericCapacity;
