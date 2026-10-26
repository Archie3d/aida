with Ada.Text_IO;
procedure GenericDerivedDiscriminants is
    type Record_Type (N : Positive) is record
        Text : String (1 .. N);
    end record;
    type Record_Child is new Record_Type;
    subtype Pair is Record_Type (2);
    subtype Child_Pair is Record_Child (2);
    generic
        type T is new Record_Type;
    procedure Change (X : in out T);
    procedure Change (X : in out T) is
    begin
        X.Text (X.Text'First) := 'A';
    end Change;
    procedure Change_Child is new Change (Record_Child);
    generic
        type T is new Pair;
    function Copy (X : T) return T;
    function Copy (X : T) return T is
    begin
        return X;
    end Copy;
    function Copy_Pair is new Copy (Child_Pair);
    package Tagged_Model is
        type Root (N : Positive) is tagged record
            Text : String (1 .. N);
        end record;
        function Length (X : Root) return Integer;
        type Child is new Root with null record;
        overriding function Length (X : Child) return Integer;
    end Tagged_Model;
    package body Tagged_Model is
        function Length (X : Root) return Integer is
        begin
            return X.N;
        end Length;
        function Length (X : Child) return Integer is
        begin
            return X.N + 10;
        end Length;
    end Tagged_Model;
    generic
        type T is new Tagged_Model.Root with private;
    function Read_Length (X : T) return Integer;
    function Read_Length (X : T) return Integer is
    begin
        return Length (X);
    end Read_Length;
    function Child_Length is new Read_Length (Tagged_Model.Child);
    procedure Check is
        X : Record_Child (3) := (N => 3, Text => "old");
        Y : Child_Pair := (N => 2, Text => "hi");
        Z : Child_Pair := Copy_Pair (Y);
        V : Tagged_Model.Child (4);
    begin
        V.Text := "test";
        Change_Child (X);
        if X.Text /= "Ald" or Z.Text /= "hi" or Child_Length (V) /= 14 then
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
    Ada.Text_IO.Put_Line ("generic derived discriminants passed");
end GenericDerivedDiscriminants;
