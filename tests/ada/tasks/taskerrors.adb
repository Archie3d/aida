with Ada.Streams;
procedure TaskErrors is
    task type Worker;
    task body Worker is
    begin
        return;
    end Worker;
    task type Missing;
    task body Orphan is
    begin
        null;
    end Orphan;
    left, right : Worker;
    copy : Worker := left;
    type Group is array (1 .. 2) of Worker;
    members, othersMembers : Group;
    type Holder is record
        item : Worker;
    end record;
    type Derived is new Holder;
    function Result return Worker;
    function Result return Worker is
    begin
        return left;
    end Result;
    type Pointer is access Worker;
    allocated : Pointer := new Worker'(left);
    stream : Integer := 0;
begin
    left := right;
    members := othersMembers;
    if left = right then
        null;
    end if;
    Worker'Read (stream, left);
end TaskErrors;
