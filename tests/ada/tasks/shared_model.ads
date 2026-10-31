package Shared_Model is
    type Flag is new Boolean;
    pragma Atomic (Flag);
    ready : Flag := False;
    value : Integer := 0;
    pragma Volatile (value);
end Shared_Model;
