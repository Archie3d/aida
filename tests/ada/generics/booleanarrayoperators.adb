with Ada.Text_IO;
procedure BooleanArrayOperators is
    procedure Check is
        type Bits is array (Integer range <>) of Boolean;
        Left : Bits (-2 .. 0) := (True, False, True);
        Right : Bits (4 .. 6) := (False, False, True);
        Calls : Integer := 0;
        function Fetch return Bits is
        begin
            Calls := Calls + 1;
            return Left;
        end Fetch;
        generic
            A, B : Bits;
            with function Op (L, R : Bits) return Bits;
        package Model is
            Value : Bits := Op (A, B);
        end Model;
        package Joined is new Model (Left, Right, "or");
        package Exclusive is new Model (B => Right, A => Left, Op => "xor");
        Empty : Bits (7 .. 6);
        Null_Result : Bits := not Empty;
        Both : Bits := Left and Right;
        Chained : Bits := (not Left) xor (not Right);
        Flipped : Bits := not Fetch;
        function Flip (Value : Bits) return Bits is
        begin
            return not Value;
        end Flip;
        Returned : Bits := Flip (Right);
        Short : Bits (1 .. 2) := (others => False);
        Failed : Boolean := False;
        function Mismatch return Bits is
        begin
            return Left and Short;
        end Mismatch;
    begin
        if Joined.Value /= Bits'(True, False, True)
            or Exclusive.Value /= Bits'(True, False, False)
            or Chained /= Bits'(True, False, False)
            or Both /= Bits'(False, False, True)
            or Flipped /= Bits'(False, True, False)
            or Returned /= Bits'(True, True, False)
            or Flipped'First /= -2 or Flipped'Last /= 0
            or Joined.Value'First /= -2 or Calls /= 1
            or Null_Result'First /= 7 or Null_Result'Length /= 0
        then
            raise Program_Error;
        end if;
        begin
            declare
                Bad : Bits := Mismatch;
            begin
                null;
            end;
        exception
            when Constraint_Error => Failed := True;
        end;
        if not Failed then raise Program_Error; end if;
    end Check;
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
            for I in 1 .. 10 loop
                Check;
            end loop;
            Results.Passed;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end Exercise;
begin
    Check;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("Boolean array operators passed");
end BooleanArrayOperators;
