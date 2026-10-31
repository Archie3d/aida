with Ada.Text_IO;
procedure GenericAccessCallbacks is
    procedure Check is
        Calls : Integer := 0;
        function Next (X : Integer) return Integer is
        begin
            Calls := Calls + 1;
            return X + 1;
        end Next;
        function Previous (X : Integer) return Integer is
        begin
            return X - 1;
        end Previous;
        type Callback is access function (X : Integer) return Integer;
        Named : Callback := Next'Access;
        Anon : access function (X : Integer) return Integer := Next'Access;
        Other : access function (X : Integer) return Integer := Previous'Access;
        generic
            Value : Callback;
            Alias_Value : in out Callback;
        function Named_Call (X : Integer) return Integer;
        function Named_Call (X : Integer) return Integer is
        begin
            Alias_Value := Previous'Access;
            return Value (X) + Alias_Value (X);
        end Named_Call;
        function Apply_Named is new Named_Call (Named, Named);
        generic
            Value : access function (X : Integer) return Integer := Next'Access;
            Alias_Value : in out access function (X : Integer) return Integer;
        function Anonymous_Call (X : Integer) return Integer;
        function Anonymous_Call (X : Integer) return Integer is
        begin
            Alias_Value := Other;
            return Value (X) + Alias_Value (X);
        end Anonymous_Call;
        function Apply_Anon is new Anonymous_Call (Alias_Value => Anon);
    begin
        if Apply_Named (10) /= 20 or Apply_Anon (20) /= 40 or Calls /= 2
            or Named (10) /= 9 or Anon (10) /= 9 then
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
    Ada.Text_IO.Put_Line ("GenericAccessCallbacks passed");
end GenericAccessCallbacks;
