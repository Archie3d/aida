with Ada.Text_IO;
procedure Returnforms is
    procedure Step (Value : in out Natural) is
    begin
        Value := Value + 1;
        return;
    end Step;
    generic
    procedure Generic_Step (Value : in out Natural);
    procedure Generic_Step (Value : in out Natural) is
    begin
        Value := Value + 2;
        return;
    end Generic_Step;
    procedure G_Step is new Generic_Step;
    function Value (N : Positive) return Natural is
    begin
        if N = 1 then
            return 10;
        elsif N = 2 then
            return Result : Natural := 20;
        else
            return Result : Natural := 30 do
                if N = 3 then
                    return;
                end if;
                Result := 40;
            end return;
        end if;
    end Value;
    generic
    function Generic_Value (N : Positive) return Natural;
    function Generic_Value (N : Positive) return Natural is
    begin
        return Result : Natural := Value (N) do
            declare
                -- This extended return applies to a different function body.
                function Nested return Natural is
                begin
                    return Inner : Natural := 1 do
                        return;
                    end return;
                end Nested;
            begin
                Result := Result + Nested;
                return;
            end;
        end return;
    end Generic_Value;
    function G_Value is new Generic_Value;
    function Handled return Natural is
    begin
        return Result : Natural := 5 do
            raise Constraint_Error;
        exception
            when Constraint_Error => return;
        end return;
    end Handled;
    protected Gate is
        procedure Step (Value : in out Natural);
        entry Open (Value : in out Natural);
        function Calls return Natural;
    private
        Count : Natural := 0;
    end Gate;
    protected body Gate is
        procedure Step (Value : in out Natural) is
        begin
            Count := Count + 1;
            Value := Value + 3;
            return;
        end Step;
        entry Open (Value : in out Natural) when True is
        begin
            Count := Count + 1;
            Value := Value + 4;
            return;
        end Open;
        function Calls return Natural is
        begin
            return Result : Natural := Count;
        end Calls;
    end Gate;
    task Worker is
        entry Step (Value : in out Natural);
        entry Finish (Finished : out Boolean);
    end Worker;
    task body Worker is
    begin
        accept Step (Value : in out Natural) do
            Value := Value + 5;
            return;
        end Step;
        accept Finish (Finished : out Boolean) do
            Finished := True;
            return;
        end Finish;
    end Worker;
    Number : Natural := 0;
    Finished : Boolean := False;
begin
    Step (Number);
    G_Step (Number);
    Gate.Step (Number);
    Gate.Open (Number);
    Worker.Step (Number);
    Worker.Finish (Finished);
    if Number /= 15 or not Finished or Gate.Calls /= 2
        or Value (1) /= 10 or Value (2) /= 20 or Value (3) /= 30 or Value (4) /= 40
        or G_Value (1) /= 11 or G_Value (2) /= 21 or G_Value (3) /= 31 or G_Value (4) /= 41
        or Handled /= 5 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("return forms passed");
end Returnforms;
