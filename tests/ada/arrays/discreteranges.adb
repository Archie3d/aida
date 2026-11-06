with Ada.Text_IO; use Ada.Text_IO;
procedure DiscreteRanges is
    procedure Check_Ranges is
        type Day is (Mon, Tue, Wed, Thu, Fri);
        subtype Midweek is Day range Tue .. Thu;
        type Vector is array (Day range <>) of Integer;
        type Matrix is array (Integer range <>, Day range <>) of Integer;
        Calls : Integer := 0;
        N : Integer := 3;
        function Source (Value : Integer := 7) return Vector is
        begin
            Calls := Calls + 1;
            return (Tue .. Thu => Value);
        end Source;
        function Bound (Value : Integer) return Integer is
        begin
            Calls := Calls + 1;
            return Value;
        end Bound;
        type Copy is array (Source (7)'Range) of Integer;
        X : Copy := (others => 5);
        type Implicit_Copy is array (Source'Range) of Integer;
        I : Implicit_Copy := (others => 0);
        type Typed_Copy is array (Day range Source (6)'Range) of Integer;
        T : Typed_Copy := (others => 0);
        G : Matrix (2 .. N, Tue .. Thu) := (others => (others => 0));
        Second : constant Integer := 2;
        type Columns is array (G'Range (Second)) of Integer;
        Y : Columns := (others => 6);
        type Dynamic is array (Bound (1) .. Bound (N)) of Integer;
        D : Dynamic := (others => 0);
        type Qualified is array (Integer'(1) .. Integer'(3)) of Integer;
        Q : Qualified := (others => 1);
        subtype Index is Integer range G'Range (1);
        subtype Days is Day range Source (10)'Range;
        subtype Static_Vector is Vector (Midweek range Tue .. Thu);
        type Static_Count is range 1 .. Static_Vector'Length;
        subtype Constrained is Vector (Midweek range Source (8)'Range);
        C : Constrained := (Source (9)'Range => 2);
        type Components is record
            A : Vector (Midweek);
            B : Vector (Midweek range Tue .. Thu);
            C : Vector (Y'Range);
        end record;
        R : Components := (others => (others => 1));
        V : Vector (Mon .. Fri) := (Mon .. Fri => 0);
        subtype Few is Integer range 1 .. 3;
        type Dependent (Limit : Integer) is record
            Data : String (Few range 1 .. Limit);
        end record;
        Empty_Record : Dependent (0);
    begin
        if Calls /= 8 or I'First /= Tue or T'Last /= Thu
            or Days'First /= Tue or Days'Last /= Thu or X'First /= Tue or X'Last /= Thu
            or Y'First /= Tue or Y'Last /= Thu or C'Length /= 3
            or Index'First /= 2 or Index'Last /= 3 or Q'Length /= 3
            or R.A'First /= Tue or R.B'Last /= Thu or R.C'Length /= 3
        then
            raise Program_Error;
        end if;
        if Empty_Record.Data'Length /= 0 then raise Program_Error; end if;
        begin
            declare
                Bad_Record : Dependent (4);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        N := 1;
        declare
            Later : Dynamic := (others => 0);
            Later_Copy : Copy := (others => 0);
        begin
            if Calls /= 8 or Later'Last /= 3 or Later_Copy'First /= Tue
                or Static_Count'Last /= 3 or D'Length /= 3
            then
                raise Program_Error;
            end if;
        end;
        V (Midweek range Tue .. Thu) := C (C'Range);
        if V (Wed) /= 2 then raise Program_Error; end if;
        declare
            Empty : Vector (Midweek range Fri .. Mon);
        begin
            if Empty'Length /= 0 or Empty'First /= Fri then raise Program_Error; end if;
            if C = (Midweek range Fri .. Mon => 0) then raise Program_Error; end if;
        end;
        begin
            declare
                Bad : Vector (Midweek range Mon .. Fri);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            V := (Midweek range Mon .. Fri => 1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            V (Mon .. Fri) := V (Midweek range Mon .. Fri);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
    end Check_Ranges;

    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            Check_Ranges;
            Results.Passed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    Check_Ranges;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Put_Line ("discrete ranges passed");
end DiscreteRanges;
