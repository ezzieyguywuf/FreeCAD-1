# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import PathScripts.PathHelix as PathHelix
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())


class TestPathHelix(PathTestUtils.PathTestBase):

    def setUp(self):
        self.doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_holes00.fcstd')
        self.job = PathJob.Create('Job', [self.doc.Body])

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        '''Verify Helix does not throw an exception.'''

        op = PathHelix.Create('Helix')
        op.Proxy.execute(op)

    def test01(self):
        '''Verify Helix generates proper holes from model'''

        op = PathHelix.Create('Helix')
        proxy = op.Proxy
        for base in op.Base:
            model = base[0]
            for sub in base[1]:
                pos = proxy.holePosition(op, model, sub)
                self.assertRoughly(pos.Length / 10, proxy.holeDiameter(op, model, sub))

    def test02(self):
        '''Verify Helix generates proper holes for rotated model'''

        self.job.ToolController[0].Tool.Diameter = 0.5

        op = PathHelix.Create('Helix')
        proxy = op.Proxy
        model = self.job.Model.Group[0]

        for deg in range(5, 360, 5):
            model.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)
            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(op, model, sub)
                    #PathLog.track(deg, pos, pos.Length)
                    self.assertRoughly(pos.Length / 10, proxy.holeDiameter(op, model, sub))


    def test03(self):
        '''Verify Helix generates proper holes for rotated base model'''

        for deg in range(5, 360, 5):
            self.tearDown()
            self.doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_holes00.fcstd')
            self.doc.Body.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)

            self.job = PathJob.Create('Job', [self.doc.Body])
            self.job.ToolController[0].Tool.Diameter = 0.5

            op = PathHelix.Create('Helix')
            proxy = op.Proxy
            model = self.job.Model.Group[0]

            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(op, model, sub)
                    #PathLog.track(deg, pos, pos.Length)
                    self.assertRoughly(pos.Length / 10, proxy.holeDiameter(op, model, sub))

