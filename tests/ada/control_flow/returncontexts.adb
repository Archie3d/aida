with Ada.Text_IO;
procedure Returncontexts is
    Calls : Natural := 0;
    procedure Bump is
        package Inner is
            procedure Add;
        end Inner;
        package body Inner is
            function One return Natural is
            begin
                return 1;
            end One;
            procedure Add is
            begin
                Calls := Calls + One;
                return;
            end Add;
        begin
            Add;
        end Inner;
    begin
        declare
            package Local is
                function Value return Natural;
            end Local;
            package body Local is
                function Value return Natural is
                begin
                    return 1;
                end Value;
            end Local;
        begin
            Calls := Calls + Local.Value;
            return;
        end;
    end Bump;
    function Value return Natural is
    begin
        return Result : Natural := 0 do
            declare
                package Local is
                    function One return Natural;
                end Local;
                package body Local is
                    function One return Natural is
                    begin
                        return 1;
                    end One;
                end Local;
            begin
                Result := Local.One;
                return;
            end;
        end return;
    end Value;
    procedure Handled is
    begin
        raise Constraint_Error;
    exception
        when Constraint_Error =>
            Calls := Calls + 1;
            return;
    end Handled;
    task Worker is
        entry Start;
        entry Finish;
    end Worker;
    task body Worker is
    begin
        accept Start do
            Bump;
            return;
        end Start;
        accept Finish;
    end Worker;
    protected Gate is
        entry Open;
    end Gate;
    protected body Gate is
        entry Open when True is
            package Local is
                function One return Natural;
            end Local;
            package body Local is
                function One return Natural is
                begin
                    return 1;
                end One;
            end Local;
        begin
            Calls := Calls + Local.One;
            return;
        end Open;
    end Gate;
begin
    Bump;
    if Calls /= 2 or Value /= 1 then
        raise Program_Error;
    end if;
    Worker.Start;
    Worker.Finish;
    Gate.Open;
    Handled;
    if Calls /= 6 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("return contexts passed");
end Returncontexts;
