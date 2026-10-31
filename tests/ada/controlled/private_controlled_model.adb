package body Private_Controlled_Model is
    procedure Finalize (Object : in out Hidden) is
    begin
        Controlled_Result_Model.Finalize (Controlled_Result_Model.Guard (Object));
    end Finalize;
    function Build return Box is
    begin
        return (Item => Make (7));
    end Build;
    procedure Adjust (Object : in out Hidden) is
    begin
        Adjustments := Adjustments + 1;
        Controlled_Result_Model.Adjust (Controlled_Result_Model.Guard (Object));
    end Adjust;
    function Make (Value : Integer) return Hidden is
        Local : Hidden;
    begin
        Local.Data.Value := Value;
        return Local;
    end Make;
    function Value (Object : Hidden) return Integer is
    begin
        return Object.Data.Value;
    end Value;
    procedure Initialize (Object : in out Limited_Hidden) is
    begin
        Limited_Live := Limited_Live + 1;
    end Initialize;
    procedure Finalize (Object : in out Limited_Hidden) is
    begin
        Limited_Live := Limited_Live - 1;
    end Finalize;
end Private_Controlled_Model;
