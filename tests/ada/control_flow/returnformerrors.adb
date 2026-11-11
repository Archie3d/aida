procedure Returnformerrors is
    procedure Extended_Procedure is
    begin
        return Result : Integer := 1 do
            Result := True;
            return;
        end return;
    end Extended_Procedure;
    procedure Valued_Procedure is
    begin
        return 1;
    end Valued_Procedure;
    protected Gate is
        procedure Step;
        entry Open;
        entry Other;
    end Gate;
    protected body Gate is
        procedure Step is
        begin
            return Result : Integer := 1;
        end Step;
        entry Open when True is
        begin
            return Result : Integer := 1;
        end Open;
        entry Other when True is
        begin
            return 1;
        end Other;
    end Gate;
    task Worker is
        entry Step;
        entry Other;
    end Worker;
    task body Worker is
    begin
        accept Step do
            return Result : Integer := 1;
        end Step;
        accept Other do
            return 1;
        end Other;
    end Worker;
    function Missing return Integer is
    begin
        return;
    end Missing;
    function Nested return Integer is
    begin
        return Result : Integer := 0 do
            if Result = 0 then
                return Inner : Integer := 1;
            else
                return 2;
            end if;
        exception
            when others => return 3;
        end return;
    end Nested;
begin
    null;
end Returnformerrors;
