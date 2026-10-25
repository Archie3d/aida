with Ada.Text_IO;
procedure DerivedScalarOperations is
    procedure Check is
        type Parent is (First, Second, Third, Fourth);
        Low : Parent := Second;
        High : Parent := Fourth;
        subtype Middle is Parent range Low .. High;
        type Choice is new Middle range Third .. High;
        type Inherited_Choice is new Middle;
        Flag : Boolean := True;
        subtype True_Only is Boolean range Flag .. Flag;
        type Derived_Boolean is new True_Only;
        type Parent_Integer is range -100 .. 100;
        Limit : Parent_Integer := 20;
        subtype Small is Parent_Integer range -Limit .. Limit;
        type Number is new Small range -10 .. 10;
        type Inherited_Number is new Small;
        subtype Static_Small is Integer range 1 .. 2;
        subtype Empty is Number range 2 .. 1;
        X : Choice := Third;
        B : Derived_Boolean := True;
        N : Number := 10;
        Wide : Number'Base;
        Caught : Integer := 0;
        function Identity (Text : String) return String is
        begin
            return Text;
        end Identity;
        procedure Accept_Number (Value : Number) is
        begin
            raise Program_Error;
        end Accept_Number;
        function Return_Number return Number is
        begin
            return Number'Value (Identity ("40"));
        end Return_Number;
    begin
        Low := First;
        High := First;
        Limit := 1;
        Flag := False;
        if Choice'First /= Third or else Choice'Last /= Fourth
            or else Inherited_Choice'First /= Second
            or else Inherited_Choice'Last /= Fourth
            or else Number'First /= -10 or else Number'Last /= 10
            or else Inherited_Number'Last /= 20
            or else Derived_Boolean'First /= True then
            raise Program_Error;
        end if;
        -- Value uses the base range for static and runtime subtypes alike.
        if Choice'Value ("FIRST") /= First
            or else Choice'Value (Identity ("SECOND")) /= Second
            or else Choice'Val (0) /= First
            or else Choice'Pred (Second) /= First
            or else Parent (Choice'Value ("FIRST")) /= First
            or else Choice'Value ("FIRST") in Choice
            or else Choice'Pos (First) /= 0 then
            raise Program_Error;
        end if;
        if Derived_Boolean'Value (Identity ("FALSE")) /= False
            or else Derived_Boolean'Val (0) /= False
            or else (not B) /= False or else (B and False) /= False
            or else (B xor True) /= False or else (False or B) /= True
            or else (B and then False) /= False
            or else (False or else B) /= True then
            raise Program_Error;
        end if;
        if Number'Value (Identity ("40")) /= 40
            or else Number'Value ("-40") /= -40
            or else Static_Small'Value ("40") /= 40
            or else Empty'Value ("40") /= 40
            or else Number'Val (40) /= 40
            or else Number'Succ (20) /= 21 or else Number'Pred (-20) /= -21
            or else N + 30 /= 40 or else N - 30 /= -20
            or else N * 3 /= 30 or else 40 / N /= 4
            or else 41 rem N /= 1 or else 41 mod N /= 1
            or else N ** 2 /= 100 or else abs Number'Val (-40) /= 40
            or else -Number'Val (-40) /= 40 or else +Number'Val (40) /= 40
            or else N >= 40 or else 40 in Number then
            raise Program_Error;
        end if;
        Wide := Number'Value ("40");
        if Wide /= 40 or else Parent_Integer (Wide) /= 40 then
            raise Program_Error;
        end if;
        -- The receiving context still enforces its first-subtype constraint.
        begin
            X := Choice'Value ("FIRST");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            B := Derived_Boolean'Value ("FALSE");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            N := Number'Value ("40");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            N := Number (Wide);
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            Accept_Number (Number'Value ("40"));
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            N := Return_Number;
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        -- Bad spellings and values beyond the base range remain checked.
        begin
            Wide := Number'Value ("2147483648");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            Wide := Number'Value ("broken");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        begin
            X := Choice'Value ("UNKNOWN");
        exception when Constraint_Error => Caught := Caught + 1;
        end;
        if Caught /= 9 or else N /= 10 or else X /= Third or else B /= True then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    exception
        when others => Ada.Text_IO.Put_Line ("worker failed");
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("derived scalar operations passed");
end DerivedScalarOperations;
