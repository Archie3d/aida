package body TaskEntryLibrary is
    task body Worker is
    begin
        accept Add (N : in out Integer) do N := N + 5; end Add;
        accept Stop;
    end Worker;
    function Make return Worker is
    begin return Result : Worker; end Make;
end TaskEntryLibrary;
