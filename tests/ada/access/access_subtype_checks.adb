package body Access_Subtype_Checks is
    Secret_Good : aliased Secret (2);
    Secret_Bad : aliased Secret (3);
    function Make_Secret (Wrong : Boolean) return Secret_Reference is
    begin
        if Wrong then return Secret_Bad'Access; end if;
        return Secret_Good'Access;
    end Make_Secret;
    procedure Run is
        Calls : Integer := 0;
        function Capture return Positive is
        begin
            Calls := Calls + 1;
            return 2;
        end Capture;
        subtype Local is Reference (Capture, Capture);
        subtype Local_Alias is Local;
        subtype Three is Reference (3, 3);
        subtype Grouped is Reference (Left | Right => Capture);
        subtype Required is not null Local;
        type Readonly is access constant Item;
        subtype Readonly_Fixed is Readonly (2, 2);
        type Mutable (D : Integer := 1) is null record;
        type Pool is access Mutable;
        subtype Pinned is Pool (2);
        type Fields is record
            A, B : Reference (Capture, Capture);
        end record;
        type Elements is array (1 .. 2) of Reference (Capture, Capture);
        type Dependent (D : Positive) is record
            Text : String (1 .. D);
            Pointer : Reference (Left => D, Right => Capture);
        end record;
        X : Local := null;
        P : Pinned := new Mutable (2);
        Nil : Reference := null;
        A, B : Fields;
        E : Elements := (others => null);
        H : Dependent (2);
        Null_H : Dependent (3);
        Source : Reference := Good'Access;
        Wrong : Reference := Bad'Access;
        Wrong_Subtype : Three := Wrong;
        Library_Value : Alias := Source;
        Derived_Value : Derived := Derived (Source);
        Constant_Value : Readonly_Fixed := Readonly_Fixed (Source);
        function Result (Value : Reference) return Local is
        begin
            return Value;
        end Result;
        procedure Take (Value : Local) is
        begin
            if Value /= null and then Value.Left /= 2 then raise Program_Error; end if;
        end Take;
        procedure Change (Value : out Reference; New_Value : Reference) is
        begin
            Value := New_Value;
        end Change;
        procedure Update (Value : in out Local) is
        begin
            Value := null;
        end Update;
    begin
        if Calls /= 11 or Limit /= 3 or Library_Value.Left /= 2
            or Derived_Value.Right /= 2 or Constant_Value.Left /= 2 or Null_H.Pointer /= null
        then
            raise Program_Error;
        end if;
        if P.D /= 2 then raise Program_Error; end if;
        begin
            P.all := (D => 3);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            X := Required (Nil);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        X := Grouped (Source);
        A.A := Source;
        B.B := Source;
        E (1) := Source;
        H.Pointer := Source;
        Take (X);
        Take (null);
        Update (X);
        if X /= null then raise Program_Error; end if;
        X := Local_Alias (null);
        X := Result (null);
        Change (X, Source);
        if X /= Source or Calls /= 11 then raise Program_Error; end if;
        begin
            X := Wrong;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if X /= Source then raise Program_Error; end if;
        begin
            X := Local (Wrong_Subtype);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Derived_Value := Derived (Wrong_Subtype);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Constant_Value := Readonly_Fixed (Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Library_Value := Alias (Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Take (Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            X := Result (Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Change (X, Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if X /= Source then raise Program_Error; end if;
        begin
            H.Pointer := Wrong;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Change (H.Pointer, Wrong);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                Invalid : Local := Wrong;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                Invalid : Dependent (3) := (D => 3, Text => "abc", Pointer => Source);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                Zero : Integer := 0;
                Invalid : Reference (Positive (Zero), 2) := null;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        if Calls /= 11 or H.Pointer /= Source then raise Program_Error; end if;
    end Run;
begin
    Limit := 3;
end Access_Subtype_Checks;
