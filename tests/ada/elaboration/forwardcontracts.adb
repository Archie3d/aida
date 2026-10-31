with Ada.Text_IO; use Ada.Text_IO;
procedure Forwardcontracts is
    function Pick (X : Integer) return Integer is
    begin
        return X + 10;
    end Pick;
    generic
        type Item is range <>;
    package Wrapper is
        type Number is new Item;
        function Run (X : Number) return Integer;
    end Wrapper;
    package body Wrapper is
        generic
            Value : Number;
            with function Transform (X : Integer) return Integer is Pick;
        function G return Integer;
        function Invoke (X : Number) return Integer is
            function Instance is new G (X);
        begin
            return Instance;
        end Invoke;
        function Pick (X : Integer) return Integer is
        begin
            return X + 20;
        end Pick;
        Offset : Integer := 1;
        function G return Integer is
            generic
                N : Number;
            function Nested return Integer;
            function Call_Nested return Integer is
                function Instance is new Nested (Value);
            begin
                return Instance;
            end Call_Nested;
            function Nested return Integer is
            begin
                return Transform (Integer (N)) + Pick (Offset);
            end Nested;
        begin
            return Call_Nested;
        end G;
        function Run (X : Number) return Integer is
        begin
            return Invoke (X);
        end Run;
    end Wrapper;
    package First is new Wrapper (Integer);
    package Second is new Wrapper (Long_Integer);
begin
    if First.Run (2) /= 33 or Second.Run (3) /= 34 then
        raise Program_Error;
    end if;
    Put_Line ("forward contract bindings ok");
end Forwardcontracts;
