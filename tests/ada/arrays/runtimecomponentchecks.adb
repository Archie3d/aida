with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure RuntimeComponentChecks is
    Calls : Integer := 0;
    function Bound return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Bound;
    type Mixed is record
        A : Integer range 0 .. Bound := 0;
        B : String (1 .. Bound) := (others => 'b');
        C : Integer range 0 .. Bound := 0;
    end record;
    Value : Mixed;
    function Make return Mixed is
    begin
        Calls := Calls + 1;
        return Value;
    end Make;
    Size : Long_Integer := Make'Size;
    N : Integer := 0;
    subtype Empty is String (4 .. N);
    type Null_Record is record
        Data : Empty;
        Last : Integer := 17;
    end record;
    X : Null_Record;
    type Pointer is access String;
    procedure Free is new Ada.Unchecked_Deallocation (String, Pointer);
    P : Pointer := new Empty;
    Caught : Integer := 0;
    procedure Mark is
    begin
        Caught := Caught + 1;
    end Mark;
begin
    if Calls /= 4 or Value.B /= "bb" or Size /= 96 or X.Last /= 17
        or Empty'Size /= 0 or P.all'First /= 4 or P.all'Last /= 0 or P.all'Length /= 0 then
        raise Program_Error;
    end if;
    Free (P);
    begin
        declare
            Bound_Value : Integer := 4;
            subtype Index is Integer range 1 .. 3;
            type Vector is array (Index range <>) of Integer;
            type Link is access Vector;
            V : Link := new Vector (1 .. Bound_Value);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Mark;
    end;
    begin
        declare
            Top : Long_Integer := Long_Integer'Last;
            type Huge is array (Long_Integer range 1 .. Top) of Integer;
            type Holder is record
                Data : Huge;
            end record;
        begin
            raise Program_Error;
        end;
    exception
        when Storage_Error => Mark;
    end;
    begin
        declare
            Top : Long_Integer := Long_Integer'Last;
            type Bytes is array (Long_Integer range <>) of Character;
            type Link is access Bytes;
            V : Link := new Bytes (0 .. Top);
        begin
            raise Program_Error;
        end;
    exception
        when Storage_Error => Mark;
    end;
    if Caught /= 3 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("runtime component checks ok");
end RuntimeComponentChecks;
