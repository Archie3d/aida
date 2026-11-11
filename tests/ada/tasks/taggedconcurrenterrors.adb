procedure TaggedConcurrentErrors is
    task type Worker;
    task body Worker is
    begin
        null;
    end Worker;
    type Root is tagged limited null record;
    type Holder is new Root with record
        member : Worker;
    end record;
    protected type Cell is
        procedure Touch;
    end Cell;
    protected body Cell is
        procedure Touch is
        begin
            null;
        end Touch;
    end Cell;
    type NonlimitedRoot is tagged null record;
    type Bad is new NonlimitedRoot with record
        taskPart : Worker;
        protectedPart : Cell;
    end record;
    first : Holder;
    second : Holder := first;
begin
    first := second;
    if first = second then
        null;
    end if;
end TaggedConcurrentErrors;
