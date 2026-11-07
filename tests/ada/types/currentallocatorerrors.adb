procedure CurrentAllocatorErrors is
    type Rec;
    type Pointer is access Rec;
    type Rec is record
        Next : Pointer := new Rec;
    end record;
begin
    null;
end CurrentAllocatorErrors;
