#!/usr/bin/env python
#
# plot-languages.py - make plots from the results of pango-language-profile
#
# Copyright (C) 2005 Federico Mena-Quintero
# federico@novell.com

import sys
import optparse
import math
import cairo
from xml.dom import minidom, Node

class Language:
    def __init__ (self, name, time, num_strings, num_chars):
        self.name = name
        self.total_time = time
        self.char_time = time / num_chars
        self.num_strings = num_strings
        self.num_chars = num_chars

def string_from_node (node):
    c = node.childNodes[0]
    if c.nodeType == Node.TEXT_NODE:
        return c.nodeValue
    else:
        return None;

def float_from_node (node):
    c = node.childNodes[0]
    if c.nodeType == Node.TEXT_NODE:
        return float (c.nodeValue)
    else:
        return None;

def long_from_node (node):
    c = node.childNodes[0]
    if c.nodeType == Node.TEXT_NODE:
        return long (c.nodeValue)
    else:
        return None;

class ResultSet:
    def __init__ (self, filename):
        self.test_name = filename    # Will be overriden by <name> in the test file, hopefully
        self.languages = {}
        self.parse (filename)

    def parse (self, filename):
        doc = minidom.parse (filename)
        node = doc.documentElement
        if node.nodeName != "pango-benchmark":
            print "xml doesn't have a pango-benchmark toplevel element"
            sys.exit (1)

        for l in node.childNodes:
            if l.nodeType != Node.ELEMENT_NODE:
                continue

            if l.nodeName == "name":
                self.test_name = string_from_node (l)
            elif l.nodeName == "language":
                self.parse_language_node (l)

    def parse_language_node (self, node):
        name = None
        time = None
        num_strings = None
        num_chars = None

        for child in node.childNodes:
            if child.nodeType != Node.ELEMENT_NODE:
                continue

            if child.nodeName == "name":
                name = string_from_node (child)
            elif child.nodeName == "elapsed":
                time = float_from_node (child)
            elif child.nodeName == "total_strings":
                num_strings = long_from_node (child)
            elif child.nodeName == "total_chars":
                num_chars = long_from_node (child)

        if (name, time, num_strings, num_chars) != (None, None, None, None) and num_strings >= 400000:
            lang = Language (name, time, num_strings, num_chars)
            self.languages[name] = lang

    def get_name (self):
        return self.test_name

    def get_languages (self):
        return self.languages

    def get_language (self, name):
        return self.languages[name]

def interp_colors (c1, c2, f):
    return (c1[0] + f * (c2[0] - c1[0]),
            c1[1] + f * (c2[1] - c1[1]),
            c1[2] + f * (c2[2] - c1[2]))

def set_source_rgb (ctx, triplet):
    ctx.set_source_rgb (triplet[0], triplet[1], triplet[2])

def time_compare_func (a, b):
    return -cmp (a.string_time, b.string_time)

def compute_miss_rate (hits, misses):
    return misses.__float__() / (hits + misses)

def compute_miss_rate_percent (hits, misses):
    return 100.0 * compute_miss_rate (hits, misses)

class ResultTable:
    def __init__ (self, languages):
        self.table = []
        self.lang_to_index_hash = {}
        self.languages = languages[:] # Copy the list

        i = 0
        for l in self.languages:
            self.lang_to_index_hash[l] = i
            i += 1

    def add_result_set (self, rset):
        row = []
        for lang_name in self.languages:
            lang = rset.get_language (lang_name)
            row.append (lang)

        self.table.append (row)

    def get_results_per_language (self, lang_name):
        column = []

        for row in self.table:
            index = self.lang_to_index_hash[lang_name]
            column.append (row[index])

        return column

    def get_results_per_set_index (self, set_index):
        return self.table[set_index][:]   # copy the row

    def get_result (self, set_index, lang_name):
        row = self.table[set_index]
        return row[self.lang_to_index_hash[lang_name]]

class LanguagePlot:
    def __init__ (self):
        # Constants for layout
        self.font_face = "Bitstream Vera Sans"
        self.font_size = 12
        self.lang_text_width = 40
        self.header_height = 20
        self.time_bar_height = 6
        self.row_spacing = 6
        self.time_bar_length = 400
        self.background_color = (1.0, 1.0, 1.0)
        self.text_color = (0.0, 0.0, 0.0)
        self.time_color1 = (0.0, 0.0, 0.36)
        self.time_color2 = (0.64, 0.64, 0.78)
        self.time_color_warn = (1.0, 0.0, 0.0)
        self.time_factor = 0.3

        # Data
        self.result_sets = []

    def add_result_set (self, rset):
        self.result_sets.append (rset)

    def get_common_languages (self):
        languages = set (self.result_sets[0].get_languages ().keys ())

        for rset in self.result_sets[1:]:
            rset_langs = set (rset.get_languages().keys ())
            languages = languages.intersection (rset_langs)

        languages = list (languages)
        languages.sort ()
        return languages

    def build_result_table (self, languages):
        result_table = ResultTable (languages)
        for rset in self.result_sets:
            result_table.add_result_set (rset)

        return result_table

    def find_max_time (self, result_table, languages):
        max_time = 0.0
        
        for i in range (0, len (self.result_sets)):
            for l in languages:
                lang = result_table.get_result (i, l)
                if lang.char_time > max_time:
                    max_time = lang.char_time

        return max_time

    def compute_size (self, languages):
        # [] test 1
        # [] test 2
        # [] timing is funny
        # lang execution time
        # zh   [################]
        #      [##############]
        # ja   [###############]
        #      [###########]
        # es   [#######]
        #      [#####]
        num_languages = len (languages)
        width = (self.lang_text_width + self.time_bar_length)
        height = (self.header_height +
                  (len (self.result_sets) + 1) * self.header_height +   # (num_result_sets + 1) for the "timing is funny" row
                  num_languages * (len (self.result_sets) * self.time_bar_height + self.row_spacing))
        return (width, height)

    def plot_test_heading (self, ctx, test_name, color, ypos):
        xpos = 0

        set_source_rgb (ctx, color)
        ctx.rectangle (xpos, ypos, self.font_size, self.font_size)
        ctx.fill ()

        xpos += self.header_height

        # Test name
        set_source_rgb (ctx, self.text_color)
        text_y = ypos + self.font_size
        ctx.move_to (xpos, text_y)
        ctx.show_text (test_name)

    def plot_language_results (self, ctx, lang_name, language_results, max_time, ypos):
        ctx.select_font_face (self.font_face)
        ctx.set_font_size (self.font_size)

        xpos = 0

        # Language name
        set_source_rgb (ctx, self.text_color)
        text_y = ypos + self.font_size
        ctx.move_to (xpos, text_y)
        ctx.show_text (lang_name)
        xpos += self.lang_text_width

        num_rsets = len (self.result_sets)

        # Time bar

        for i in range (0, num_rsets):
            if num_rsets == 1:
                c = self.time_color1
            else:
                if i > 0 and language_results[i - 1].char_time < language_results[i].char_time:
                    c = self.time_color_warn
                else:
                    c = interp_colors (self.time_color1, self.time_color2, i.__float__() / (num_rsets - 1))

            set_source_rgb (ctx, c)

            factor = language_results[i].char_time / max_time
            w = self.time_bar_length * factor

            ctx.rectangle (xpos,
                           ypos,
                           w,
                           self.time_bar_height)
            ctx.fill ()
            ypos += self.time_bar_height

    def plot (self, out_filename):
        languages = self.get_common_languages ()
        result_table = self.build_result_table (languages)
        num_rsets = len (self.result_sets)

        base_timings = []
        for lang_name in languages:
            time = result_table.get_result (0, lang_name).char_time
            base_timings.append ((lang_name, time))

        base_timings.sort (lambda a, b : cmp (b[1], a[1]))

        max_time = self.find_max_time (result_table, languages)

        (width, height) = self.compute_size (languages)
        surface = cairo.ImageSurface (cairo.FORMAT_RGB24, width, height)

        ctx = cairo.Context (surface)
        ctx.select_font_face (self.font_face, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
        ctx.set_font_size (self.font_size)

        # Background

        set_source_rgb (ctx, self.background_color)
        ctx.rectangle (0, 0, width, height);
        ctx.fill ()

        ypos = 0

        # Headings

        for i in range (0, num_rsets):
            test_name = self.result_sets[i].get_name ()

            if num_rsets == 1:
                c = self.time_color1
            else:
                c = interp_colors (self.time_color1, self.time_color2, i.__float__() / (num_rsets - 1))

            self.plot_test_heading (ctx, test_name, c, ypos)
            ypos += self.header_height

        self.plot_test_heading (ctx, "Timing should be lower than preceding one, but it isn't", self.time_color_warn, ypos)
        ypos += self.header_height

        # Header

        set_source_rgb (ctx, self.text_color)
        text_y = ypos + (self.header_height - self.font_size) / 2.0 + self.font_size

        x = 0
        ctx.move_to (x, text_y)
        ctx.show_text ("Lang")
        x += self.lang_text_width

        ctx.move_to (x, text_y)
        ctx.show_text ("Avg. time per character")
        x += self.time_bar_length

        ypos += self.header_height

        # Languages

        for timing in base_timings:
            lang_name = timing[0]
            language_results = result_table.get_results_per_language (lang_name)
            self.plot_language_results (ctx, lang_name, language_results, max_time, ypos)
            ypos += num_rsets * self.time_bar_height + self.row_spacing

        # Done!

        surface.write_to_png (out_filename)


def main ():
    option_parser = optparse.OptionParser (usage="usage: %prog -o output.png <results-file.xml> ...")
    option_parser.add_option ("-o", "--output", dest="output", metavar="FILE",
                              help="Name of output file (output is a PNG file)")

    (options, args) = option_parser.parse_args ()

    if not options.output:
        print 'Please specify an output filename with "-o file.png" or "--output=file.png".'
        sys.exit (1)

    in_filenames = args
    out_filename = options.output

    if len (in_filenames) == 0:
        print "Please specify at least one input file."
        sys.exit (1)

    plot = LanguagePlot ()

    for file in args:
        rset = ResultSet (file)
        plot.add_result_set (rset)

    plot.plot (out_filename)


main ()
