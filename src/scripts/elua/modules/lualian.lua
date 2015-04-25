-- Elua lualian module

local cutil  = require("cutil")
local util   = require("util")
local log    = require("eina.log")
local eolian = require("eolian")

local M = {}

local dom

cutil.init_module(function()
    dom = log.Domain("lualian")
    if not dom:is_valid() then
        log.err("Could not register log domain: lualian")
        error("Could not register log domain: lualian")
    end
end, function()
    dom:unregister()
    dom = nil
end)

local lua_kw = {
    ["and"] = true, ["end"] = true, ["in"] = true, ["local"] = true,
    ["nil"] = true, ["not"] = true, ["or"] = true, ["repeat"] = true,
    ["then"] = true, ["until"] = true
}

local kw_t = function(n)
    if lua_kw[n] then
        return n .. "_"
    end
    return n
end

local int_builtin = {
    ["byte" ] = true, ["short"] = true, ["int"] = true, ["long"] = true,
    ["llong"] = true,

    ["int8"  ] = true, ["int16"] = true, ["int32"] = true, ["int64"] = true,
    ["int128"] = true,

    ["intptr"] = true
}

local num_others = {
    ["size" ] = true, ["ssize" ] = true, ["ptrdiff"] = true,
    ["float"] = true, ["double"] = true
}

local is_num = function(x)
    if num_others [x       ] then return true end
    if int_builtin[x       ] then return true end
    if int_builtin["u" .. x] then return true end
    return false
end

local known_out = {
    ["Evas_Coord"] = function(expr) return ("tonumber(%s)"):format(expr) end,
    ["bool"] = function(expr) return ("((%s) ~= 0)"):format(expr) end,
    ["char"] = function(expr) return ("string.char(%s)"):format(expr) end
}

local known_in = {
    ["Evas_Coord"] = function(expr) return expr end,
    ["bool"] = function(expr) return expr end
}

local known_ptr_out = {
    ["const char"] = function(expr) return ("ffi.string(%s)"):format(expr) end
}

local known_ptr_in = {
    ["const char"] = function(expr) return expr end
}

local convfuncs = {}

local build_calln = function(tps, expr, isin)
    return expr
end

local typeconv = function(tps, expr, isin)
    if tps:type_get() == eolian.type_type.POINTER then
        local base = tps:base_type_get()
        local f = (isin and known_ptr_in or known_ptr_out)[base:c_type_get()]
        if f then return f(expr) end
        return build_calln(tps, expr, isin)
    end

    local tp = tps:name_get()

    if is_num(tp) then
        return isin and expr or ("tonumber(%s)"):format(expr)
    end

    local f = (isin and known_in or known_out)[tp]
    if f then
        return f(expr)
    end

    return build_calln(tps, expr, isin)
end

local Node = util.Object:clone {
    generate = function(self, s)
    end,

    gen_children = function(self, s)
        local len = #self.children

        local evs = self.events
        local evslen
        if evs then evslen = #evs end
        local hasevs = evs and evslen > 0

        local hasprops = false
        local nprops = 0
        local props = {}

        for i, v in ipairs(self.children) do
            v.parent_node = self
            if v.generate_prop then
                if v:generate_prop(props) then
                    nprops = nprops + 1
                end
                hasprops = true
            end
            v:generate(s, (not hasevs) and (not hasprops) and (i == len))
        end

        if hasevs then
            s:write("    __events = {\n")
            for i, v in ipairs(evs) do
                v.parent_node = self
                v:generate(s, i == evslen)
            end
            s:write("    }", hasprops and "," or "", "\n")
        end

        if hasprops then
            if hasevs then
                s:write("\n")
            end
            s:write("    __properties = {\n")
            local pi = 0
            for k, v in pairs(props) do
                pi = pi + 1
                s:write("        [\"", k, "\"] = { ", table.concat(v, ", "),
                    " }", pi ~= nprops and "," or "", "\n")
            end
            s:write("    }\n")
        end
    end
}

local Method = Node:clone {
    __ctor = function(self, meth)
        self.method = meth
    end,

    gen_proto = function(self)
        if self.cached_proto then return self.cached_proto end

        local meth = self.method
        local pars = meth:parameters_get()
        local rett = meth:return_type_get(eolian.function_type.METHOD)

        local proto = {
            name    = meth:name_get()
        }
        proto.ret_type = rett and rett:c_type_get() or "void"
        local args, cargs, vargs = { "self" }, {}, {}
        proto.args, proto.cargs, proto.vargs = args, cargs, vargs
        local rets = {}
        proto.rets = rets
        local allocs = {}
        proto.allocs = allocs

        proto.full_name = meth:full_c_name_get()

        local dirs = eolian.parameter_dir

        local fulln = proto.full_name

        if rett then
            rets[#rets + 1] = typeconv(rett, "v", false)
        end

        for v in pars do
            local dir, tps, nm = v:direction_get(), v:type_get(), kw_t(v:name_get())
            local tp = tps:c_type_get()
            if dir == dirs.OUT or dir == dirs.INOUT then
                if dir == dirs.INOUT then
                    args[#args + 1] = nm
                end
                cargs [#cargs  + 1] = tp .. " *" .. nm
                vargs [#vargs  + 1] = nm
                allocs[#allocs + 1] = { tp, nm, (dir == dirs.INOUT)
                    and typeconv(tps, nm, true) or nil }
                rets  [#rets   + 1] = typeconv(tps, nm .. "[0]", false)
            else
                args  [#args   + 1] = nm
                cargs [#cargs  + 1] = tp .. " " .. nm
                vargs [#vargs  + 1] = typeconv(tps, nm, true)
            end
        end

        if #cargs == 0 then cargs[1] = "void" end

        self.cached_proto = proto

        return proto
    end,

    generate = function(self, s, last)
        local proto = self:gen_proto()
        s:write("    ", proto.name, proto.suffix or "", " = function(",
            table.concat(proto.args, ", "), ")\n")
        s:write( "        eo.__do_start(self, __class)\n")
        for i, v in ipairs(proto.allocs) do
            s:write("        local ", v[2], " = ffi.new(\"", v[1], "[1]\")\n")
        end
        local genv = (proto.ret_type ~= "void")
        s:write("        ", genv and "local v = " or "", "__lib.",
            proto.full_name, "(", table.concat(proto.vargs, ", "), ")\n")
        s:write("        eo.__do_end()\n")
        if #proto.rets > 0 then
            s:write("        return ", table.concat(proto.rets, ", "), "\n")
        end
        s:write("    end", last and "" or ",", last and "\n" or "\n\n")
    end,

    gen_ffi = function(self, s)
        local proto = self:gen_proto()
        local ret = proto.ret_type
        if ret:match("[a-zA-Z0-9_]$") then
            ret = ret .. " "
        end
        local cproto = {
            "    ", ret, proto.full_name, "(", table.concat(proto.cargs, ", "),
            ");\n"
        }
        s:write(table.concat(cproto))
    end
}

local Property = Method:clone {
    __ctor = function(self, prop, ftype)
        self.property = prop
        self.isget    = (ftype == eolian.function_type.PROP_GET)
        self.ftype    = ftype
    end,

    gen_proto = function(self)
        if self.cached_proto then return self.cached_proto end

        local prop = self.property
        local keys = prop:property_keys_get():to_array()
        local vals = prop:property_values_get():to_array()
        local rett = prop:return_type_get(self.ftype)

        local proto = {
            name    = prop:name_get(),
            suffix  = (self.isget and "_get" or "_set"),
            nkeys   = #keys,
            nvals   = #vals
        }
        proto.ret_type = rett and rett:c_type_get() or "void"
        local args, cargs, vargs = { "self" }, {}, {}
        proto.args, proto.cargs, proto.vargs = args, cargs, vargs
        local rets = {}
        proto.rets = rets
        local allocs = {}
        proto.allocs = allocs

        proto.full_name = prop:full_c_name_get() .. proto.suffix

        local dirs = eolian.parameter_dir

        local fulln = proto.full_name
        if #keys > 0 then
            for i, v in ipairs(keys) do
                local nm  = kw_t(v:name_get())
                local tps = v:type_get()
                local tp  = tps:c_type_get()
                args [#args  + 1] = nm
                cargs[#cargs + 1] = tp .. " " .. nm
                vargs[#vargs + 1] = typeconv(tps, nm, true)
            end
        end
        proto.kprop = #keys > 0

        if #vals > 0 then
            if self.isget then
                if #vals == 1 and not rett then
                    local tps = vals[1]:type_get()
                    proto.ret_type = tps:c_type_get()
                    rets[#rets + 1] = typeconv(tps, "v", false)
                else
                    for i, v in ipairs(vals) do
                        local dir, tps, nm = v:direction_get(), v:type_get(),
                            kw_t(v:name_get())
                        local tp = tps:c_type_get()
                        cargs [#cargs  + 1] = tp .. " *" .. nm
                        vargs [#vargs  + 1] = nm
                        allocs[#allocs + 1] = { tp, nm }
                        rets  [#rets   + 1] = typeconv(tps, nm .. "[0]", false)
                    end
                end
            else
                for i, v in ipairs(vals) do
                    local dir, tps, nm = v:direction_get(), v:type_get(),
                        kw_t(v:name_get())
                    local tp = tps:c_type_get()
                    args [#args  + 1] = nm
                    cargs[#cargs + 1] = tp .. " " .. nm
                    vargs[#vargs + 1] = typeconv(tps, nm, true)
                end
            end
        end

        if #cargs == 0 then cargs[1] = "void" end

        self.cached_proto = proto

        return proto
    end,

    generate_prop = function(self, props)
        local proto = self:gen_proto()
        local prop = props[proto.name]
        if prop then
            if self.isget then
                prop[3] = "true"
            else
                prop[4] = "true"
            end
            return false
        else
            props[proto.name] = { proto.nkeys, math.max(proto.nvals, 1),
                tostring(self.isget), tostring(not self.isget) }
            return true
        end
    end
}

local Event = Node:clone {
    __ctor = function(self, ename, etype, edesc, ecname)
        self.ename  = ename
        self.etype  = etype
        self.edesc  = edesc
        self.ecname = ecname
    end,

    generate = function(self, s, last)
        s:write("        [\"", self.ename, "\"] = __lib.",
            "_" .. self.ecname, last and "\n" or ",\n")
    end,

    gen_ffi = function(self, s)
        s:write("    extern const Eo_Event_Description ",
            "_" .. self.ecname, ";\n")
    end
}

local gen_ns = function(klass, s)
    local nspaces = klass:namespaces_get():to_array()
    if #nspaces > 1 then
        local lnspaces = {}
        for i = 2, #nspaces do
            lnspaces[i - 1] = '"' .. nspaces[i]:lower() .. '"'
        end
        s:write("local __M = util.get_namespace(M, { ",
            table.concat(lnspaces, ", "), " })\n")
        return "__M"
    else
        return "M"
    end
end

local Mixin = Node:clone {
    __ctor = function(self, klass, ch, evs)
        self.klass    = klass
        self.children = ch
        self.events   = evs
    end,

    generate = function(self, s)
        dom:log(log.level.INFO, "  Generating for interface/mixin: "
            .. self.klass:full_name_get())

        s:write("ffi.cdef [[\n")
        self:gen_ffi(s)
        s:write("]]\n\n")

        gen_ns(self.klass, s)

        s:write("__body = {\n")
        self:gen_children(s)
        s:write("}\n")
    end,

    gen_ffi = function(self, s)
        s:write("    const Eo_Class *", self.klass:c_get_function_name_get(),
            "(void);\n")
        for i, v in ipairs(self.children) do
            v.parent_node = self
            v:gen_ffi(s)
        end
        if self.events then
            for i, v in ipairs(self.events) do
                v.parent_node = self
                v:gen_ffi(s)
            end
        end
    end
}

local build_pn = function(fn, pn)
    if fn == pn then
        return kw_t(pn)
    end
    return fn .. "_" .. pn
end

local Class = Node:clone {
    __ctor = function(self, klass, parent, mixins, ch, evs)
        self.klass      = klass
        self.parent     = parent
        self.interfaces = interfaces
        self.mixins     = mixins
        self.children   = ch
        self.events     = evs
    end,

    generate = function(self, s)
        dom:log(log.level.INFO, "  Generating for class: "
            .. self.klass:full_name_get())

        s:write("ffi.cdef [[\n")
        self:gen_ffi(s)
        s:write("]]\n\n")

        local mname = gen_ns(self.klass, s)

        s:write("__body = {\n")
        self:gen_ctor(s)
        self:gen_children(s)
        s:write("}\n")

        -- write the constructor
        s:write(([[

%s.%s = function(parent, ...)
    return eo.__ctor_common(__class, parent, eo.class_get("%s").__eo_ctor,
                            1, ...)
end
]]):format(mname, self.klass:name_get(), self.klass:full_name_get():gsub("%.",
        "_")))
    end,

    gen_ffi = Mixin.gen_ffi,

    gen_ctor = function(self, s)
        local ctors = self.klass:constructors_get()
        if not ctors then return end
        -- collect constructor information
        local ftp = eolian.function_type
        local dir = eolian.parameter_dir
        s:write("    __eo_ctor = function(self, ")
        local cfuncs, parnames, upars = {}, {}, {}
        for ctor in ctors do
            local cfunc = ctor:function_get()
            local cn = cfunc:name_get()
            local tp = cfunc:type_get()
            if tp == ftp.PROPERTY or tp == ftp.PROP_SET or tp == ftp.METHOD then
                cfuncs[#cfuncs + 1] = cfunc
                if tp ~= ftp.METHOD then
                    for par in cfunc:property_keys_get() do
                        parnames[#parnames + 1] = build_pn(cn, par:name_get())
                    end
                end
                for par in cfunc:parameters_get() do
                    if par:direction_get() ~= dir.OUT then
                        parnames[#parnames + 1] = build_pn(cn, par:name_get())
                    end
                end
            end
        end
        s:write(table.concat(parnames, ", "))
        if #parnames == 0 then
            s:write("__func")
        else
            s:write(", __func")
        end
        s:write(")\n")
        -- write ctor body
        local j = 1
        for i, cfunc in ipairs(cfuncs) do
            s:write("        self:", cfunc:name_get())
            if cfunc:type_get() ~= ftp.METHOD then
                s:write("_set")
            end
            s:write("(")
            local fpars = {}
            if cfunc:type_get() ~= ftp.METHOD then
                for par in cfunc:property_keys_get() do
                    fpars[#fpars + 1] = parnames[j]
                    j = j + 1
                end
            end
            for par in cfunc:parameters_get() do
                if par:direction_get() ~= dir.OUT then
                    fpars[#fpars + 1] = parnames[j]
                    j = j + 1
                end
            end
            s:write(table.concat(fpars, ", "))
            s:write(")\n")
        end
        s:write("        if __func then __func() end\n")
        s:write("    end")
        if #self.children > 0 then
            s:write(",\n\n")
        else
            s:write("\n")
        end
    end
}

local File = Node:clone {
    __ctor = function(self, fname, klass, ch)
        self.fname    = fname:match(".+/(.+)") or fname
        self.klass    = klass
        self.children = ch
    end,

    generate = function(self, s)
        local kls  = self.klass
        local ckls = self.children[1]

        local kn  = kls:full_name_get()
        local par = ckls.parent

        dom:log(log.level.INFO, "Generating for file: " .. self.fname)
        dom:log(log.level.INFO, "  Class            : " .. kn)

        local knu = kn:gsub("%.", "_")
        local paru = par and ('"' .. par:gsub("%.", "_") .. '"') or "nil"

        s:write(([[
-- EFL LuaJIT bindings: %s (class %s)
-- For use with Elua; automatically generated, do not modify

local cutil = require("cutil")
local util  = require("util")
local ffi   = require("ffi")
local eo    = require("eo")

local M, __lib = ...

local __class
local __body

local init = function()
    __class = __lib.%s()
    eo.class_register("%s", %s, __body, __class)
]]):format(self.fname, kn, kls:c_get_function_name_get(), knu, paru))

        if ckls.mixins then for i, v in ipairs(ckls.mixins) do
            s:write(("    eo.class_mixin(\"%s\", \"%s\")\n"):format(knu,
                v:gsub("%.", "_")))
        end end

        s:write([[
end

cutil.init_module(init, function() end)

]])

        self:gen_children(s)

        s:write([[

return M
]])

        local first = true
        for name, v in pairs(convfuncs) do
            if first then
                print("\nRequired conversion functions:")
                first = false
            end
            print("    " .. name)
        end
    end
}

local gen_contents = function(klass)
    local cnt = {}
    local ft  = eolian.function_type
    -- first try properties
    local props = klass:functions_get(ft.PROPERTY):to_array()
    for i, v in ipairs(props) do
        if v:scope_get() == eolian.object_scope.PUBLIC and not v:is_c_only() then
            local ftype  = v:type_get()
            local fread  = (ftype == ft.PROPERTY or ftype == ft.PROP_GET)
            local fwrite = (ftype == ft.PROPERTY or ftype == ft.PROP_SET)
            if fwrite then
                cnt[#cnt + 1] = Property(v, ft.PROP_SET)
            end
            if fread then
                cnt[#cnt + 1] = Property(v, ft.PROP_GET)
            end
        end
    end
    -- then methods
    local meths = klass:functions_get(ft.METHOD):to_array()
    for i, v in ipairs(meths) do
        if v:scope_get() == eolian.object_scope.PUBLIC and not v:is_c_only() then
            cnt[#cnt + 1] = Method(v)
        end
    end
    -- events
    local evs = {}
    local events = klass:events_get():to_array()
    for i, v in ipairs(events) do
        evs[#evs + 1] = Event(v:name_get(), v:type_get(), v:description_get(),
            v:c_name_get())
    end
    return cnt, evs
end

local gen_mixin = function(klass)
    return Mixin(klass, gen_contents(klass))
end

local gen_class = function(klass)
    local inherits = klass:inherits_get():to_array()
    local parent
    local mixins = {}
    local ct = eolian.class_type
    local n = 1
    if inherits[n] then
        local tp = eolian.class_get_by_name(inherits[n]):type_get()
        if tp == ct.REGULAR or tp == ct.ABSTRACT then
            parent = inherits[n]
            n = n + 1
        end
    end
    for i = n, #inherits do
        local v = inherits[i]
        local tp = eolian.class_get_by_name(v):type_get()
        if tp == ct.UNKNOWN then
            error(klass:full_name_get() .. ": unknown inherit " .. v)
        end
        mixins[#mixins + 1] = v
    end
    return Class(klass, parent, mixins, gen_contents(klass))
end

M.include_dir = function(dir)
    if not eolian.directory_scan(dir) then
        error("Failed including directory: " .. dir)
    end
end

M.load_eot_files = function()
    return eolian.all_eot_files_parse()
end

M.system_directory_scan = function()
    return eolian.system_directory_scan()
end

M.generate = function(fname, fstream)
    if not eolian.eo_file_parse(fname) then
        error("Failed parsing file: " .. fname)
    end
    if not eolian.database_validate() then
        error("Failed validating database.")
    end
    local sfn = fname:match(".*[\\/](.+)$") or fname
    local klass = eolian.class_get_by_file(sfn)
    local tp = klass:type_get()
    local ct = eolian.class_type
    local cl
    if tp == ct.MIXIN or tp == ct.INTERFACE then
        cl = gen_mixin(klass)
    elseif tp == ct.REGULAR or tp == ct.ABSTRACT then
        cl = gen_class(klass)
    else
        error(klass:full_name_get() .. ": unknown type")
    end
    File(fname, klass, { cl }):generate(fstream or io.stdout)
end

return M
