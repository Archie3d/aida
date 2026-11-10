procedure Race is
    Shared : Integer := 0;
    procedure Start;
    pragma Import (C, Start, "raceStart");
    procedure Write_Turn;
    pragma Import (C, Write_Turn, "raceWriteTurn");
    procedure Finish;
    pragma Import (C, Finish, "raceFinish");
    task type Worker;
    task body Worker is
    begin
        Start;
        Write_Turn;
        Shared := 1;
        Finish;
    end Worker;
    Workers : array (1 .. 2) of Worker;
begin
    null;
end Race;
