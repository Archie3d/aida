with Ada.Text_IO;
with Ada.Finalization;
procedure GenericAccessLifetime is
    procedure Check is
        Live : Integer := 0;
        Calls : Integer := 0;
        package Resources is
            type Item is new Ada.Finalization.Controlled with null record;
            overriding procedure Initialize (Object : in out Item);
            overriding procedure Finalize (Object : in out Item);
        end Resources;
        package body Resources is
            procedure Initialize (Object : in out Item) is
            begin
                Live := Live + 1;
            end Initialize;
            procedure Finalize (Object : in out Item) is
            begin
                Live := Live - 1;
            end Finalize;
        end Resources;
        generic
            Value : access Resources.Item := new Resources.Item;
            Guard : Integer := 0;
        package Holder is
            function Present return Boolean;
        end Holder;
        package body Holder is
            function Present return Boolean is
            begin
                return Value /= null;
            end Present;
        end Holder;
        function Reject return Integer is
        begin
            raise Program_Error;
            return 0;
        end Reject;
        type Pointer is access Integer;
        subtype Non_Null is not null Pointer;
        function Null_Value return Pointer is
        begin
            Calls := Calls + 1;
            return null;
        end Null_Value;
        generic
            Value : Non_Null := Null_Value;
        package Named is end Named;
        function Null_Anonymous return access Integer is
        begin
            Calls := Calls + 1;
            return null;
        end Null_Anonymous;
        generic
            Value : not null access Integer := Null_Anonymous;
        package Anonymous is end Anonymous;
        A : aliased Integer := 8;
        P : access Integer := A'Access;
        Saved : access Integer;
        generic
            Actual : access Integer;
        package Saver is
            procedure Save;
        end Saver;
        package body Saver is
            procedure Save is
            begin
                Saved := Actual;
            end Save;
        end Saver;
        package Safe is new Saver (A'Access);
        procedure Escape (Value : access Integer) is
            generic
                Actual : access Integer;
            package Model is
                procedure Save;
            end Model;
            package body Model is
                procedure Save is
                begin
                    Actual.all := Actual.all + 1;
                end Save;
            end Model;
            package Instance is new Model (Value);
        begin
            Instance.Save;
        end Escape;
    begin
        declare
            package Instance is new Holder;
        begin
            if Live /= 1 or not Instance.Present then raise Constraint_Error; end if;
        end;
        if Live /= 0 then raise Constraint_Error; end if;
        begin
            declare
                package Instance is new Holder;
            begin
                if Live /= 1 or not Instance.Present then raise Constraint_Error; end if;
                raise Program_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 then raise Constraint_Error; end if;
        begin
            declare
                package Instance is new Holder (Guard => Reject);
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 then raise Constraint_Error; end if;
        begin
            declare
                package Instance is new Named;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                package Instance is new Named (Null_Value);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                package Instance is new Anonymous;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                package Instance is new Anonymous (Null_Anonymous);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        if Calls /= 4 then raise Program_Error; end if;
        -- A borrowed access parameter may be forwarded into an instance
        -- inside its call; the designation remains valid throughout that call.
        declare
            B : aliased Integer := 9;
        begin
            Escape (B'Access);
            if B /= 10 then raise Program_Error; end if;
        end;
        Escape (A'Access);
        if P.all /= 9 then raise Program_Error; end if;
        declare
            B : aliased Integer := 15;
            package Deeper is new Saver (B'Access);
        begin
            begin
                Deeper.Save;
                raise Constraint_Error;
            exception
                when Program_Error => null;
            end;
        end;
        if Saved /= null then raise Program_Error; end if;
        Safe.Save;
        if Saved.all /= 9 then raise Program_Error; end if;
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
    Ada.Text_IO.Put_Line ("access object lifetimes passed");
end GenericAccessLifetime;
