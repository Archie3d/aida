procedure Race is
    Shared : Integer := 0;
    protected Gate is
        procedure Arrive;
        entry Go;
    private
        Arrived : Integer := 0;
    end Gate;
    protected body Gate is
        procedure Arrive is
        begin
            Arrived := Arrived + 1;
        end Arrive;
        entry Go when Arrived = 2 is
        begin
            null;
        end Go;
    end Gate;
    task type Worker;
    task body Worker is
    begin
        Gate.Arrive;
        Gate.Go;
        for I in 1 .. 100000 loop
            Shared := I;
        end loop;
    end Worker;
    Workers : array (1 .. 2) of Worker;
begin
    null;
end Race;
