with Ada.Text_IO;
with GenericPackageModel;
with GenericPartialPackageLibrary;
procedure GenericPartialPackageLibrary_Main is
    procedure Check is
        Calls : Integer := 0;
        function Seed return Integer is
        begin
            Calls := Calls + 1;
            return 3;
        end Seed;
        Initial : constant Integer := Seed;
        package Data is new GenericPackageModel (Integer, Initial);
        package View is new GenericPartialPackageLibrary (Data);
        package Nested is new View.Inner (Data);
        generic
            with package P is new GenericPackageModel (Item => Integer, Initial => <>);
        package Forwarder is
            package Forwarded is new GenericPartialPackageLibrary (P);
        end Forwarder;
        package Forwarded_View is new Forwarder (Data);
        generic
            with package Signature is new GenericPackageModel (others => <>);
            Value : Signature.Element;
        package Containers is
            function Read return Signature.Element;
        end Containers;
        package body Containers is
            function Read return Signature.Element is
            begin
                return Value;
            end Read;
        end Containers;
        package Container is new Containers (Data, Initial);
        generic
            with package P is new Containers (Signature => Data, Value => <>);
        function Read_Container return Integer;
        function Read_Container return Integer is
        begin
            return P.Read;
        end Read_Container;
        function Read_Value is new Read_Container (Container);
    begin
        if Calls /= 1 or Read_Value /= 3 or View.Read /= 3 or Nested.Read /= 3 or Forwarded_View.Forwarded.Read /= 3 then
            raise Program_Error;
        end if;
        Data.Write (9);
        View.Reset;
        if Data.Read /= 3 or Calls /= 1 then
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
    Ada.Text_IO.Put_Line ("library partial formal packages passed");
end GenericPartialPackageLibrary_Main;
