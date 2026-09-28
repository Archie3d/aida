with Ada.Text_IO; use Ada.Text_IO;
procedure PrimitiveChecks is
    package P is
        type T is range 0 .. 100;
        subtype Small is T range 0 .. 5;
        procedure Set (X : out T);
        procedure Fail (X : in out T);
        function Read (X : Small) return Integer;
        function Defaulted (X : T := 3) return T;
        function Read (X : T; Extra : Integer) return Integer;
    end P;
    package body P is
        procedure Set (X : out T) is
        begin
            X := 80;
        end Set;
        procedure Fail (X : in out T) is
        begin
            X := 90;
            raise Program_Error;
        end Fail;
        function Read (X : Small) return Integer is
        begin
            return Integer (X);
        end Read;
        function Defaulted (X : T := 3) return T is
        begin
            return X;
        end Defaulted;
        function Read (X : T; Extra : Integer) return Integer is
        begin
            return Integer (X) + Extra;
        end Read;
    end P;
    type D is new P.Small;
    X : D := 2;
    Wide : D'Base := 80;
    generic
        Offset : Integer;
    package G is
        type T is new P.T;
        function Test return Integer;
    end G;
    package body G is
        function Test return Integer is
            X : T := 2;
        begin
            return Read (X) + Read (X, Offset);
        end Test;
    end G;
    package Instance is new G (10);
begin
    Put_Line (Integer'Image (Integer (Defaulted)));
    Set (Wide);
    Put_Line (Integer'Image (Integer (Wide)));
    begin
        Set (X);
    exception
        when Constraint_Error => Put_Line ("copy back checked");
    end;
    Put_Line (Integer'Image (Integer (X)));
    begin
        Put_Line (Integer'Image (Read (Wide)));
    exception
        when Constraint_Error => Put_Line ("formal checked");
    end;
    begin
        Fail (X);
    exception
        when Program_Error => Put_Line ("exception propagated");
    end;
    Put_Line (Integer'Image (Integer (X)));
    Put_Line (Integer'Image (Instance.Test));
end PrimitiveChecks;
