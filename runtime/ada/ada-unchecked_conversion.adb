function Ada.Unchecked_Conversion (S : Source) return Target is
begin
    -- Calls to this predefined instance use intrinsic representation copying.
    raise Program_Error;
end Ada.Unchecked_Conversion;
