with Ada.Text_IO;
procedure GenericPackagePartial is
    procedure Check is
        Counter : Integer := 0;
        generic
            type T is private;
            Seed : T;
            N : Integer := 3;
            with function Step (Value : Integer) return Integer is Integer'Succ;
            Target : in out Integer;
        package Model is
            subtype Element is T;
            function Initial return T;
        end Model;
        package body Model is
            function Initial return T is
            begin
                return Seed;
            end Initial;
        end Model;
        package Data is new Model (Integer, 7, 9, Integer'Pred, Counter);
        generic
            with package P is new Model (T => Integer, Seed => 7, others => <>);
        package Views is
            function Read return Integer;
            procedure Advance;
        end Views;
        package body Views is
            function Read return Integer is
            begin
                return P.Initial + P.N;
            end Read;
            procedure Advance is
            begin
                P.Target := P.Step (P.Target);
            end Advance;
        end Views;
        package View is new Views (Data);
        generic
            with package P is new Model (Integer, 7, N => <>, Target => Counter);
        function Defaulted return Integer;
        function Defaulted return Integer is
        begin
            return P.Step (P.N);
        end Defaulted;
        function Read_Defaulted is new Defaulted (Data);
        generic
            with package P is new Model (others => <>);
        function Read_All return P.T;
        function Read_All return P.T is
        begin
            return P.Initial;
        end Read_All;
        function Read_Data is new Read_All (Data);
    begin
        if View.Read /= 16 or Read_Defaulted /= 8 or Read_Data /= 7 then
            raise Program_Error;
        end if;
        View.Advance;
        if Counter /= -1 then
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
    Ada.Text_IO.Put_Line ("partial formal package matching passed");
end GenericPackagePartial;
