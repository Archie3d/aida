with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Scalar_Dynamic_Model; use Scalar_Dynamic_Model;
procedure ScalarDynamic is
    Calls : Integer := 0;
    function Upper return Integer is
    begin
        Calls := Calls + 1;
        return Calls * 10;
    end Upper;
    X, Y : Integer range 1 .. Upper := 1;
    type Pair is record
        A, B : Integer range 1 .. Upper := 1;
    end record;
    P : Pair;
    H : Holder;
    A : Vector := (others => 3);
    Caught : Integer := 0;
    type Pointer is access Small;
    procedure Free is new Ada.Unchecked_Deallocation (Small, Pointer);
    Heap : Pointer;
    Bound : Float := 2.0;
    Anonymous_Real : Float range -Bound .. Bound := 1.0;
    function Wrong_Real return Real_Range is
    begin
        return 5.0;
    end Wrong_Real;
    procedure Mark is
    begin
        Caught := Caught + 1;
    end Mark;
    procedure Recurse (N : Integer) is
        Top : Float := Float (N);
        subtype Local_Real is Float range -Top .. Top;
        subtype Alias_Real is Local_Real;
        Value : Local_Real := 1.0;
        function Echo return Alias_Real is
        begin
            return Value;
        end Echo;
    begin
        Top := 100.0;
        if Local_Real'First /= -Float (N) or else Alias_Real'Last /= Float (N)
            or else Float (N + 1) in Local_Real then
            raise Program_Error;
        end if;
        if N > 1 then
            Recurse (N - 1);
        end if;
        if Echo /= 1.0 or else Local_Real'Last /= Float (N) then
            raise Program_Error;
        end if;
        begin
            Value := Float (N + 1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
    end Recurse;
begin
    if Deferred /= 3 or else Calls /= 4 or else Small'Last /= 12 or else Small'Width /= 3
        or else Real_Range'First /= -4.5 or else Real_Range'Last /= 4.5
        or else Captured.Bound'Last /= 12 or else Captured.Bound'Width /= 3
        or else Fixed_Range'Last /= 2.0 or else Echo (3) /= 3
        or else Real_Echo (2.0) /= 2.0 then
        raise Program_Error;
    end if;
    begin X := 11; exception when Constraint_Error => Mark; end;
    begin Y := 21; exception when Constraint_Error => Mark; end;
    Y := 11;
    begin P.A := 31; exception when Constraint_Error => Mark; end;
    begin P.B := 41; exception when Constraint_Error => Mark; end;
    P.B := 31;
    begin H.Value := 13; exception when Constraint_Error => Mark; end;
    begin H.Real_Value := 5.0; exception when Constraint_Error => Mark; end;
    begin A (1) := 13; exception when Constraint_Error => Mark; end;
    begin X := Echo (13); exception when Constraint_Error => Mark; end;
    begin H.Real_Value := Real_Echo (5.0); exception when Constraint_Error => Mark; end;
    begin Heap := new Integer'(13); exception when Constraint_Error => Mark; end;
    Heap := new Small'(4);
    if Heap.all /= 4 then raise Program_Error; end if;
    Free (Heap);
    begin Anonymous_Real := 3.0; exception when Constraint_Error => Mark; end;
    begin
        declare
            subtype Tiny is Float range 1.0 .. 2.0;
            subtype Bad is Tiny range 0.0 .. 1.0;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Mark;
    end;
    begin H.Real_Value := Wrong_Real; exception when Constraint_Error => Mark; end;
    begin H.Real_Value := Real_Range (5.0); exception when Constraint_Error => Mark; end;
    begin H.Real_Value := Real_Range'(5.0); exception when Constraint_Error => Mark; end;
    if Caught /= 15 or else X /= 1 or else Y /= 11 or else H.Value /= 3 then
        raise Program_Error;
    end if;
    declare
        subtype Static_Real is Float range 1.0 .. 2.0;
    begin
        if 1.5 not in Static_Real or else 3.0 in Static_Real then raise Program_Error; end if;
    end;
    Recurse (4);
    declare
        Bound : Float := 2.0;
        subtype Parent is Float range 1.0 .. Bound;
        subtype Empty is Parent range 4.0 .. Bound;
    begin
        if Empty'First /= 4.0 or else Empty'Last /= 2.0 or else 3.0 in Empty then
            raise Program_Error;
        end if;
        begin
            declare
                subtype Bad is Parent range 0.0 .. Bound;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end;
    declare
        type Word is mod 2 ** 64;
        Last : Word := Word'Last;
        subtype Tail is Word range Last - 1 .. Last;
        Low : Long_Integer := Long_Integer'First;
        subtype Wide is Long_Integer range Low .. -1;
        type Choice is (A, Longest_Name, B);
        Last_Choice : Choice := B;
        subtype Choices is Choice range A .. Last_Choice;
        Flag : Boolean := True;
        subtype Flags is Boolean range False .. Flag;
        subtype Empty_Flags is Boolean range Flag .. False;
        Letter : Character := 'Z';
        subtype Letters is Character range 'A' .. Letter;
    begin
        if Tail'Width /= 21 or else Wide'Width /= 20 or else Choices'Width /= 12
            or else Flags'Width /= 5 or else Empty_Flags'Width /= 0 or else Letters'Width /= 3 then
            raise Program_Error;
        end if;
    end;
    declare
        Top : Integer := 3;
        type Derived is new Integer range 1 .. Top;
        Value : Derived := 1;
        Edge : Fraction := 2.0;
        Fixed_Object : Fraction range -Edge .. Edge := 1.0;
        type Money is delta 0.01 digits 6;
        Amount : Money := 1.0;
        Decimal_Object : Money digits 3 range -Amount .. Amount := 0.5;
    begin
        Top := 100;
        Edge := 10.0;
        Amount := 10.0;
        if Derived'Last /= 3 or else Derived'Width /= 2 then raise Program_Error; end if;
        begin Value := 4; raise Program_Error; exception when Constraint_Error => null; end;
        begin Fixed_Object := 2.25; raise Program_Error; exception when Constraint_Error => null; end;
        begin Decimal_Object := 1.01; raise Program_Error; exception when Constraint_Error => null; end;
        begin
            declare
                Invalid : Money digits 3 range -12.0 .. 12.0;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end;
    Ada.Text_IO.Put_Line ("dynamic scalar constraints");
end ScalarDynamic;
