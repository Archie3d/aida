with Ada.Unchecked_Deallocation;
procedure DeallocationFormalErrors is
    type Integer_Link is access Integer;
    type Float_Link is access Float;
    type Callback is access procedure;
    procedure Wrong_Kind is new Ada.Unchecked_Deallocation (Integer, Integer);
    procedure Wrong_Target is new Ada.Unchecked_Deallocation (Integer, Float_Link);
    procedure Wrong_Callback is new Ada.Unchecked_Deallocation (Integer, Callback);
    procedure Valid is new Ada.Unchecked_Deallocation (Integer, Integer_Link);
begin
    null;
end DeallocationFormalErrors;
