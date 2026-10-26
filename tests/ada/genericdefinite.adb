with Ada.Text_IO;
procedure GenericDefinite is
    generic
        type T (<>) is private;
    package Query is
        subtype Alias is T;
        function Value return Boolean;
    end Query;
    package body Query is
        function Value return Boolean is
        begin
            return T'Definite and Alias'Definite;
        end Value;
    end Query;
    type Open_Record (D : Integer) is null record;
    type Default_Record (D : Integer := 1) is null record;
    subtype Closed_Record is Open_Record (1);
    type Root is tagged null record;
    Bound : Integer := 3;
    subtype Dynamic_String is String (1 .. Bound);
    package A is new Query (Integer);
    package B is new Query (String);
    package C is new Query (Dynamic_String);
    package D is new Query (Open_Record);
    package E is new Query (Default_Record);
    package F is new Query (Closed_Record);
    package G is new Query (Root'Class);
    package H is new Query (Root);
    procedure Check is
    begin
        if not A.Value or B.Value or not C.Value or D.Value or not E.Value
            or not F.Value or G.Value or not H.Value then
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
    Ada.Text_IO.Put_Line ("generic definiteness passed");
end GenericDefinite;
