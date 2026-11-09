package TaskEntryLibrary is
    task type Worker is
        entry Add (N : in out Integer);
        entry Stop;
    end Worker;
    function Make return Worker;
end TaskEntryLibrary;
