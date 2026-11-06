with System.Address_To_Access_Conversions;
procedure AddressConversionErrors is
    package P is new System.Address_To_Access_Conversions (Integer);
    package Q is new System.Address_To_Access_Conversions (Integer);
    package Strings is new System.Address_To_Access_Conversions (String);
    type Variant (D : Boolean := False) is record
        Value : Integer;
    end record;
    package Variants is new System.Address_To_Access_Conversions (Variant);
    type Root is tagged null record;
    package Classes is new System.Address_To_Access_Conversions (Root'Class);
    Count : Integer := 3;
    type Dynamic is array (1 .. Count) of Integer;
    package Dynamic_Arrays is new System.Address_To_Access_Conversions (Dynamic);
    task type Worker;
    task body Worker is
    begin
        null;
    end Worker;
    package Tasks is new System.Address_To_Access_Conversions (Worker);
    protected type Lock is
        procedure Work;
    end Lock;
    protected body Lock is
        procedure Work is
        begin
            null;
        end Work;
    end Lock;
    package Locks is new System.Address_To_Access_Conversions (Lock);
    type Callback is access function (Value : System.Address) return P.Object_Pointer;
    type Back_Callback is access function (Value : P.Object_Pointer) return System.Address;
    Forward : Callback := P.To_Pointer'Access;
    Backward : Back_Callback := P.To_Address'Access;
    function Alias (Value : System.Address) return P.Object_Pointer renames P.To_Pointer;
    Renamed : Callback := Alias'Access;
    function Bad (Value : Integer) return P.Object_Pointer renames P.To_Pointer;
    A : P.Object_Pointer;
    B : Q.Object_Pointer;
begin
    A := B;
    A := Q.To_Pointer (System.Null_Address);
    A := P.To_Pointer (1);
end AddressConversionErrors;
