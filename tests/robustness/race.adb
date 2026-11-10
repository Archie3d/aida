procedure Race is
    Shared : Integer := 0;
    procedure Start;
    pragma Import (C, Start, "raceStart");
    procedure Finish;
    pragma Import (C, Finish, "raceFinish");
    task type Worker;
    task body Worker is
    begin
        Start;
        for I in 1 .. 100000 loop
            Shared := I;
        end loop;
        Finish;
    end Worker;
    Workers : array (1 .. 2) of Worker;
begin
    null;
end Race;
