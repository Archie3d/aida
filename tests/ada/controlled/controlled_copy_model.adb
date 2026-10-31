package body Controlled_Copy_Model is
    procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    procedure Adjust (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    procedure Assign (Target : in out Guard; Source : Guard) is
    begin
        Target := Source;
    end Assign;
end Controlled_Copy_Model;
