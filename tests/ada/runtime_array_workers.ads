package Runtime_Array_Workers is
    N : Integer := 4;
    protected type Counter is
        procedure Increment;
    private
        Count : Integer := 0;
    end Counter;
    type Counters is array (Integer range <>) of Counter;
    Cells : Counters (1 .. N);
    task type Worker;
    type Group is array (1 .. N) of Worker;
    Workers : Group;
end Runtime_Array_Workers;
