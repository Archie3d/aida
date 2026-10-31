with Ada.Text_IO;
procedure GenericPackages is
    generic
        type Item is private;
        Initial : Item;
    package Storage is
        subtype Element is Item;
        Value : Item := Initial;
        function Read return Item;
        procedure Write (Object : Item);
    private
        Secret : Integer := 42;
    end Storage;
    package body Storage is
        function Read return Item is
        begin
            return Value;
        end Read;
        procedure Write (Object : Item) is
        begin
            Value := Object;
        end Write;
    end Storage;
    generic
        with package Data is new Storage (<>);
    package Reader is
        function Read return Data.Item;
        procedure Reset;
    end Reader;
    package body Reader is
        function Read return Data.Item is
        begin
            return Data.Read;
        end Read;
        procedure Reset is
        begin
            Data.Write (Data.Initial);
        end Reset;
    end Reader;
    generic
        with package Data is new Storage (<>);
    package Forwarder is
        package View is new Reader (Data);
    end Forwarder;
    procedure Check is
        package Numbers is new Storage (Integer, 3);
        package Characters is new Storage (Character, 'a');
        package Number_View is new Reader (Numbers);
        package Character_View is new Reader (Characters);
        package Forwarded is new Forwarder (Numbers);
    begin
        Numbers.Write (7);
        if Forwarded.View.Read /= 7 or Number_View.Read /= 7 or Character_View.Read /= 'a' then
            raise Program_Error;
        end if;
        Number_View.Reset;
        if Numbers.Read /= 3 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("formal package views passed");
end GenericPackages;
