function Ada.Unchecked_Conversion (S : Source) return Target is
begin
    -- Calls to this predefined instance use intrinsic representation copying.
    raise Program_Error;
    -- Keep an applicable return in this generic body, even on its fallback path.
    return Unchecked_Conversion (S);
end Ada.Unchecked_Conversion;
