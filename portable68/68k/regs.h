
struct reg_state {
	union {
		u16 w;
		struct { u8 l, h; };
		struct {
			bool c : 1; bool v : 1; bool z : 1; bool n : 1;
			bool x : 1; bool no_use_bit5 : 1; bool no_use_bit6 : 1; bool no_use_bit7 : 1;
            u8 intr : 3; bool no_use_bit11 : 1; bool no_use_bit12 : 1; bool s : 1; bool no_use_bit14 : 1; bool trace : 1;
		};
	};
	inline operator u16() const { return w; }
	inline u16 operator   = (u16 data) { return w   = data; }
	inline u16 operator  |= (u16 data) { return w  |= data; }
	inline u16 operator  ^= (u16 data) { return w  ^= data; }
	inline u16 operator  &= (u16 data) { return w  &= data; }
	reg_state() : w(0) {}
};

struct reg_32 {
	union {
		u32 d;
		struct { u16 w, no_use_w; };
		struct { u8 l, no_use_h, no_use_lb, no_use_hb; };
	};
	inline u32 operator = (u32 data) { return d = data; }
	inline u32 operator -= (u32 data) { return d -= data; }
	inline u32 operator += (u32 data) { return d += data; }
	inline u32 operator |= (u32 data) { return d += data; }
	inline operator u32() const { return d; }
	reg_32() : d(0) {}
};

u16 reg_irc, reg_ird, reg_ir;
u32 reg_pc, reg_usp, reg_ssp;

reg_32 reg_d[8];
reg_32 reg_a[8];
reg_state reg_s;
